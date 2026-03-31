/* rewind.c – in-memory rewind buffer with LZ4 + delta compression.
 *
 * Adapted from NextUI's minarch.c (LoveRetro/NextUI, commit 7d201cf).
 * Original implementation Copyright (C) NextUI contributors.
 *
 * Uses:
 *   current_core.retro_serialize_size / retro_serialize / retro_unserialize
 *   PA_INFO / PA_WARN / PA_ERROR  (main.h)
 *   plat_get_ticks_ms()           (libpicofe/linux/plat.c)
 *   frame_rate                    (core.h)
 *   rewind_* config variables     (options.h)
 */

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <lz4.h>

#include "core.h"
#include "libpicofe/plat.h"
#include "main.h"
#include "options.h"
#include "rewind.h"

/* ── tuneable defaults ─────────────────────────────────────── */
#define REWIND_ENTRY_SIZE_HINT       4096
#define REWIND_MIN_ENTRIES           8
#define REWIND_POOL_SIZE_SMALL       3
#define REWIND_POOL_SIZE_LARGE       4
#define REWIND_LARGE_STATE_THRESHOLD (2 * 1024 * 1024)
#define REWIND_MAX_BUFFER_MB         256
#define REWIND_MAX_LZ4_ACCELERATION  64

/* ── entry / context structs ───────────────────────────────── */
typedef struct {
	size_t  offset;
	size_t  size;
	uint8_t is_keyframe;
} RewindEntry;

typedef struct {
	uint8_t      *buffer;
	size_t        capacity;
	size_t        head;
	size_t        tail;

	RewindEntry  *entries;
	int           entry_capacity;
	int           entry_head;
	int           entry_tail;
	int           entry_count;

	uint8_t      *state_buf;
	size_t        state_size;
	uint8_t      *scratch;
	size_t        scratch_size;

	/* delta compression */
	uint8_t      *prev_state_enc;
	uint8_t      *prev_state_dec;
	uint8_t      *delta_buf;
	int           has_prev_enc;
	int           has_prev_dec;

	int           granularity_frames;
	int           interval_ms;
	uint32_t      last_push_ms;
	uint32_t      last_step_ms;
	int           playback_interval_ms;
	int           use_time_cadence;
	int           frame_counter;
	unsigned int  generation;
	int           enabled;
	int           audio;
	int           compress;
	int           lz4_acceleration;
	int           logged_first;

	/* async worker */
	pthread_t       worker;
	pthread_mutex_t lock;
	pthread_mutex_t queue_mx;
	pthread_cond_t  queue_cv;
	int             worker_stop;
	int             worker_running;
	int             locks_ready;

	uint8_t     **capture_pool;
	unsigned int *capture_gen;
	uint8_t      *capture_busy;
	int           pool_size;
	int           free_count;
	int          *free_stack;

	int  queue_capacity;
	int  queue_head;
	int  queue_tail;
	int  queue_count;
	int *queue;
} RewindContext;

typedef enum {
	REWIND_BUF_EMPTY    = 0,
	REWIND_BUF_HAS_DATA = 1,
	REWIND_BUF_FULL     = 2,
} RewindBufferState;

/* ── module-level state ────────────────────────────────────── */
static RewindContext rewind_ctx   = {0};
static int rewind_warn_empty      = 0;

int rewinding = 0;

/* ── forward declarations ──────────────────────────────────── */
static void *Rewind_worker_thread(void *arg);
static int   Rewind_write_entry_locked(const uint8_t *compressed, size_t dest_len, int is_keyframe);
static int   Rewind_compress_state(const uint8_t *src, size_t *dest_len, int *is_keyframe_out);
static void  Rewind_wait_for_worker_idle(void);

/* ── helpers ───────────────────────────────────────────────── */
static RewindBufferState Rewind_buffer_state_locked(void)
{
	if (rewind_ctx.entry_count == 0)     return REWIND_BUF_EMPTY;
	if (rewind_ctx.head == rewind_ctx.tail) return REWIND_BUF_FULL;
	return REWIND_BUF_HAS_DATA;
}

static size_t Rewind_free_space_locked(void)
{
	RewindBufferState state = Rewind_buffer_state_locked();
	if (state == REWIND_BUF_FULL)  return 0;
	if (state == REWIND_BUF_EMPTY) return rewind_ctx.capacity;
	if (rewind_ctx.head >= rewind_ctx.tail)
		return rewind_ctx.capacity - (rewind_ctx.head - rewind_ctx.tail);
	return rewind_ctx.tail - rewind_ctx.head;
}

static void Rewind_drop_oldest_locked(void)
{
	if (!rewind_ctx.entry_count) return;
	RewindEntry *e = &rewind_ctx.entries[rewind_ctx.entry_tail];
	rewind_ctx.tail = (e->offset + e->size) % rewind_ctx.capacity;
	rewind_ctx.entry_tail = (rewind_ctx.entry_tail + 1) % rewind_ctx.entry_capacity;
	rewind_ctx.entry_count -= 1;
	if (rewind_ctx.entry_count == 0)
		rewind_ctx.head = rewind_ctx.tail = 0;
}

static int Rewind_entry_overlaps_range(int entry_idx, size_t range_start, size_t range_end)
{
	RewindEntry *e = &rewind_ctx.entries[entry_idx];
	return (e->offset < range_end) && (range_start < e->offset + e->size);
}

static void Rewind_wait_for_worker_idle(void)
{
	if (!rewind_ctx.worker_running || !rewind_ctx.pool_size) return;
	pthread_mutex_lock(&rewind_ctx.queue_mx);
	while (rewind_ctx.queue_count > 0 || rewind_ctx.free_count < rewind_ctx.pool_size) {
		pthread_mutex_unlock(&rewind_ctx.queue_mx);
		struct timespec ts = {0, 1000000}; /* 1 ms */
		nanosleep(&ts, NULL);
		pthread_mutex_lock(&rewind_ctx.queue_mx);
	}
	pthread_mutex_unlock(&rewind_ctx.queue_mx);
}

/* ── ring-buffer write ─────────────────────────────────────── */
static int Rewind_write_entry_locked(const uint8_t *compressed, size_t dest_len, int is_keyframe)
{
	if (dest_len >= rewind_ctx.capacity) {
		PA_ERROR("Rewind: state does not fit in buffer\n");
		return 0;
	}

	if (rewind_ctx.entry_count == rewind_ctx.entry_capacity)
		Rewind_drop_oldest_locked();

	size_t write_offset = rewind_ctx.head;

	if (write_offset + dest_len > rewind_ctx.capacity) {
		write_offset = 0;
		rewind_ctx.head = 0;
		if (rewind_ctx.entry_count == 0)
			rewind_ctx.tail = 0;
	}

	while (rewind_ctx.entry_count > 0) {
		int oldest_idx = rewind_ctx.entry_tail;
		if (Rewind_entry_overlaps_range(oldest_idx, write_offset, write_offset + dest_len))
			Rewind_drop_oldest_locked();
		else
			break;
	}

	while (rewind_ctx.entry_count > 0 && Rewind_free_space_locked() <= dest_len)
		Rewind_drop_oldest_locked();

	if (Rewind_free_space_locked() <= dest_len && rewind_ctx.entry_count > 0) {
		PA_ERROR("Rewind: unable to make room for entry (need %zu, have %zu)\n",
		         dest_len, Rewind_free_space_locked());
		return 0;
	}

	memcpy(rewind_ctx.buffer + write_offset, compressed, dest_len);

	RewindEntry *e = &rewind_ctx.entries[rewind_ctx.entry_head];
	e->offset      = write_offset;
	e->size        = dest_len;
	e->is_keyframe = is_keyframe ? 1 : 0;

	rewind_ctx.head = write_offset + dest_len;
	if (rewind_ctx.head >= rewind_ctx.capacity)
		rewind_ctx.head = 0;

	rewind_ctx.entry_head = (rewind_ctx.entry_head + 1) % rewind_ctx.entry_capacity;
	if (rewind_ctx.entry_count < rewind_ctx.entry_capacity)
		rewind_ctx.entry_count += 1;
	else
		Rewind_drop_oldest_locked();

	rewind_warn_empty = 0;
	return 1;
}

/* ── compression ───────────────────────────────────────────── */
static int Rewind_compress_state(const uint8_t *src, size_t *dest_len, int *is_keyframe_out)
{
	if (!rewind_ctx.scratch || !dest_len) return -1;
	if (is_keyframe_out) *is_keyframe_out = 1;

	if (!rewind_ctx.compress) {
		*dest_len = rewind_ctx.state_size;
		memcpy(rewind_ctx.scratch, src, rewind_ctx.state_size);
		if (!rewind_ctx.logged_first) {
			rewind_ctx.logged_first = 1;
			PA_INFO("Rewind: compression disabled, storing %zu bytes per snapshot\n",
			        rewind_ctx.state_size);
		}
		return 0;
	}

	const uint8_t *compress_src = src;
	int used_delta = 0;
	if (rewind_ctx.has_prev_enc && rewind_ctx.prev_state_enc && rewind_ctx.delta_buf) {
		size_t sz    = rewind_ctx.state_size;
		uint8_t *dst = rewind_ctx.delta_buf;
		const uint8_t *prev = rewind_ctx.prev_state_enc;
		for (size_t i = 0; i < sz; i++)
			dst[i] = src[i] ^ prev[i];
		compress_src = dst;
		used_delta   = 1;
	}

	int accel = rewind_ctx.lz4_acceleration > 0 ? rewind_ctx.lz4_acceleration : 2;
	int res   = LZ4_compress_fast((const char *)compress_src,
	                               (char *)rewind_ctx.scratch,
	                               (int)rewind_ctx.state_size,
	                               (int)rewind_ctx.scratch_size,
	                               accel);
	if (res <= 0) return -1;
	*dest_len = (size_t)res;

	if (is_keyframe_out)
		*is_keyframe_out = used_delta ? 0 : 1;

	if (rewind_ctx.prev_state_enc) {
		memcpy(rewind_ctx.prev_state_enc, src, rewind_ctx.state_size);
		rewind_ctx.has_prev_enc = 1;
	}
	return 0;
}

/* ── async worker ──────────────────────────────────────────── */
static void *Rewind_worker_thread(void *arg)
{
	(void)arg;
	while (1) {
		pthread_mutex_lock(&rewind_ctx.queue_mx);
		while (!rewind_ctx.worker_stop && rewind_ctx.queue_count == 0)
			pthread_cond_wait(&rewind_ctx.queue_cv, &rewind_ctx.queue_mx);

		if (rewind_ctx.worker_stop && rewind_ctx.queue_count == 0) {
			pthread_mutex_unlock(&rewind_ctx.queue_mx);
			break;
		}

		int slot = rewind_ctx.queue[rewind_ctx.queue_head];
		rewind_ctx.queue_head = (rewind_ctx.queue_head + 1) % rewind_ctx.queue_capacity;
		rewind_ctx.queue_count -= 1;
		unsigned int gen = rewind_ctx.capture_gen[slot];
		pthread_mutex_unlock(&rewind_ctx.queue_mx);

		if (gen != rewind_ctx.generation) {
			pthread_mutex_lock(&rewind_ctx.queue_mx);
			rewind_ctx.capture_busy[slot] = 0;
			rewind_ctx.free_stack[rewind_ctx.free_count++] = slot;
			pthread_mutex_unlock(&rewind_ctx.queue_mx);
			continue;
		}

		size_t dest_len  = rewind_ctx.scratch_size;
		int is_keyframe  = 1;
		pthread_mutex_lock(&rewind_ctx.lock);
		if (gen == rewind_ctx.generation) {
			int res = Rewind_compress_state(rewind_ctx.capture_pool[slot], &dest_len, &is_keyframe);
			if (res == 0)
				Rewind_write_entry_locked(rewind_ctx.scratch, dest_len, is_keyframe);
			else
				PA_ERROR("Rewind: compression failed (%i)\n", res);
		}
		pthread_mutex_unlock(&rewind_ctx.lock);

		pthread_mutex_lock(&rewind_ctx.queue_mx);
		rewind_ctx.capture_busy[slot] = 0;
		rewind_ctx.free_stack[rewind_ctx.free_count++] = slot;
		pthread_mutex_unlock(&rewind_ctx.queue_mx);
	}
	return NULL;
}

/* ── public API ────────────────────────────────────────────── */

void rewind_free(void)
{
	if (rewind_ctx.worker_running) {
		pthread_mutex_lock(&rewind_ctx.queue_mx);
		rewind_ctx.worker_stop = 1;
		pthread_cond_signal(&rewind_ctx.queue_cv);
		pthread_mutex_unlock(&rewind_ctx.queue_mx);
		pthread_join(rewind_ctx.worker, NULL);
		rewind_ctx.worker_running = 0;
	}

	if (rewind_ctx.capture_pool) {
		for (int i = 0; i < rewind_ctx.pool_size; i++) {
			if (rewind_ctx.capture_pool[i])
				free(rewind_ctx.capture_pool[i]);
		}
		free(rewind_ctx.capture_pool);
	}
	if (rewind_ctx.capture_gen)  free(rewind_ctx.capture_gen);
	if (rewind_ctx.capture_busy) free(rewind_ctx.capture_busy);
	if (rewind_ctx.free_stack)   free(rewind_ctx.free_stack);
	if (rewind_ctx.queue)        free(rewind_ctx.queue);
	if (rewind_ctx.buffer)       free(rewind_ctx.buffer);
	if (rewind_ctx.entries)      free(rewind_ctx.entries);
	if (rewind_ctx.state_buf)    free(rewind_ctx.state_buf);
	if (rewind_ctx.scratch)      free(rewind_ctx.scratch);
	if (rewind_ctx.prev_state_enc) free(rewind_ctx.prev_state_enc);
	if (rewind_ctx.prev_state_dec) free(rewind_ctx.prev_state_dec);
	if (rewind_ctx.delta_buf)    free(rewind_ctx.delta_buf);

	if (rewind_ctx.locks_ready) {
		pthread_mutex_destroy(&rewind_ctx.lock);
		pthread_mutex_destroy(&rewind_ctx.queue_mx);
		pthread_cond_destroy(&rewind_ctx.queue_cv);
	}
	memset(&rewind_ctx, 0, sizeof(rewind_ctx));
	rewinding = 0;
}

void rewind_reset(void)
{
	if (!rewind_ctx.enabled) return;

	Rewind_wait_for_worker_idle();
	pthread_mutex_lock(&rewind_ctx.lock);
	rewind_ctx.head = rewind_ctx.tail = 0;
	rewind_ctx.entry_head = rewind_ctx.entry_tail = rewind_ctx.entry_count = 0;
	rewind_ctx.has_prev_enc = 0;
	rewind_ctx.has_prev_dec = 0;
	pthread_mutex_unlock(&rewind_ctx.lock);

	rewind_ctx.frame_counter  = 0;
	rewind_ctx.last_push_ms   = 0;
	rewind_ctx.last_step_ms   = 0;
	rewind_ctx.generation    += 1;
	rewind_ctx.worker_stop    = 0;
	if (!rewind_ctx.generation)
		rewind_ctx.generation = 1;

	if (rewind_ctx.pool_size) {
		pthread_mutex_lock(&rewind_ctx.queue_mx);
		while (rewind_ctx.queue_count > 0) {
			int slot = rewind_ctx.queue[rewind_ctx.queue_head];
			rewind_ctx.queue_head = (rewind_ctx.queue_head + 1) % rewind_ctx.queue_capacity;
			rewind_ctx.queue_count -= 1;
			rewind_ctx.capture_busy[slot] = 0;
		}
		rewind_ctx.queue_head = rewind_ctx.queue_tail = 0;
		rewind_ctx.free_count = 0;
		for (int i = 0; i < rewind_ctx.pool_size; i++) {
			if (!rewind_ctx.capture_busy[i] && rewind_ctx.free_count < rewind_ctx.pool_size)
				rewind_ctx.free_stack[rewind_ctx.free_count++] = i;
		}
		pthread_mutex_unlock(&rewind_ctx.queue_mx);
	}
	rewinding        = 0;
	rewind_warn_empty = 0;
}

void rewind_init(size_t state_size)
{
	rewind_free();

	if (!rewind_enabled) return;

	if (!state_size) {
		PA_INFO("Rewind: core reported zero serialize size, disabling\n");
		return;
	}

	int buf_mb = rewind_buffer_mb;
	if (buf_mb < 1)               buf_mb = 1;
	if (buf_mb > REWIND_MAX_BUFFER_MB) buf_mb = REWIND_MAX_BUFFER_MB;

	rewind_ctx.capacity = (size_t)buf_mb * 1024 * 1024;
	rewind_ctx.compress = rewind_compress;

	if (!rewind_ctx.compress && rewind_ctx.capacity <= state_size) {
		PA_WARN("Rewind: raw snapshots (%zu bytes) do not fit in %zu-byte buffer; "
		        "falling back to compression\n",
		        state_size, rewind_ctx.capacity);
		rewind_ctx.compress = 1;
	}

	int accel = rewind_lz4_acceleration;
	if (accel < 1)                         accel = 1;
	if (accel > REWIND_MAX_LZ4_ACCELERATION) accel = REWIND_MAX_LZ4_ACCELERATION;
	rewind_ctx.lz4_acceleration = accel;
	rewind_ctx.logged_first     = 0;

	if (rewind_ctx.compress)
		PA_INFO("Rewind: config enable=1 bufferMB=%i interval=%ims audio=%i "
		        "compression=lz4 (accel=%i)\n",
		        buf_mb, rewind_interval_ms, rewind_audio, rewind_ctx.lz4_acceleration);
	else
		PA_INFO("Rewind: config enable=1 bufferMB=%i interval=%ims audio=%i "
		        "compression=raw\n",
		        buf_mb, rewind_interval_ms, rewind_audio);

	rewind_ctx.buffer = calloc(1, rewind_ctx.capacity);
	if (!rewind_ctx.buffer) {
		PA_ERROR("Rewind: failed to allocate buffer\n");
		return;
	}

	rewind_ctx.state_size = state_size;
	rewind_ctx.state_buf  = calloc(1, state_size);
	if (!rewind_ctx.state_buf) {
		PA_ERROR("Rewind: failed to allocate state buffer\n");
		rewind_free();
		return;
	}

	rewind_ctx.scratch_size = rewind_ctx.compress
	                        ? (size_t)LZ4_compressBound((int)state_size)
	                        : state_size;
	rewind_ctx.scratch = calloc(1, rewind_ctx.scratch_size);
	if (!rewind_ctx.scratch) {
		PA_ERROR("Rewind: failed to allocate scratch buffer\n");
		rewind_free();
		return;
	}

	rewind_ctx.prev_state_enc = calloc(1, state_size);
	rewind_ctx.prev_state_dec = calloc(1, state_size);
	rewind_ctx.delta_buf      = calloc(1, state_size);
	if (!rewind_ctx.prev_state_enc || !rewind_ctx.prev_state_dec || !rewind_ctx.delta_buf) {
		PA_ERROR("Rewind: failed to allocate delta buffers\n");
		rewind_free();
		return;
	}
	rewind_ctx.has_prev_enc = 0;
	rewind_ctx.has_prev_dec = 0;

	int entry_cap = (int)(rewind_ctx.capacity / REWIND_ENTRY_SIZE_HINT);
	if (entry_cap < REWIND_MIN_ENTRIES) entry_cap = REWIND_MIN_ENTRIES;
	rewind_ctx.entry_capacity = entry_cap;
	rewind_ctx.entries = calloc((size_t)entry_cap, sizeof(RewindEntry));
	if (!rewind_ctx.entries) {
		PA_ERROR("Rewind: failed to allocate entry table\n");
		rewind_free();
		return;
	}

	rewind_ctx.use_time_cadence = 1;
	rewind_ctx.interval_ms      = rewind_interval_ms < 1 ? 1 : rewind_interval_ms;

	double fps       = frame_rate > 1.0 ? frame_rate : 60.0;
	int    frame_ms  = (int)(1000.0 / fps);
	if (frame_ms < 1) frame_ms = 1;
	int capture_ms   = rewind_ctx.interval_ms;
	if (capture_ms < frame_ms) capture_ms = frame_ms;
	rewind_ctx.playback_interval_ms = capture_ms > frame_ms ? capture_ms : frame_ms;

	PA_INFO("Rewind: capture_ms=%d, playback_ms=%d "
	        "(state=%zu bytes, buffer=%zu bytes, entries=%d)\n",
	        capture_ms, rewind_ctx.playback_interval_ms,
	        state_size, rewind_ctx.capacity, entry_cap);

	rewind_ctx.audio      = rewind_audio;
	rewind_ctx.enabled    = 1;
	rewind_ctx.generation = 1;
	rewind_ctx.worker_stop = 0;

	pthread_mutex_init(&rewind_ctx.lock,     NULL);
	pthread_mutex_init(&rewind_ctx.queue_mx, NULL);
	pthread_cond_init(&rewind_ctx.queue_cv,  NULL);
	rewind_ctx.locks_ready = 1;

	rewind_ctx.pool_size = (state_size > REWIND_LARGE_STATE_THRESHOLD)
	                     ? REWIND_POOL_SIZE_LARGE : REWIND_POOL_SIZE_SMALL;
	if (rewind_ctx.pool_size < 1) rewind_ctx.pool_size = 1;

	rewind_ctx.capture_pool = calloc((size_t)rewind_ctx.pool_size, sizeof(uint8_t *));
	rewind_ctx.capture_gen  = calloc((size_t)rewind_ctx.pool_size, sizeof(unsigned int));
	rewind_ctx.capture_busy = calloc((size_t)rewind_ctx.pool_size, sizeof(uint8_t));
	rewind_ctx.free_stack   = calloc((size_t)rewind_ctx.pool_size, sizeof(int));
	rewind_ctx.queue        = calloc((size_t)rewind_ctx.pool_size, sizeof(int));
	if (!rewind_ctx.capture_pool || !rewind_ctx.capture_gen ||
	    !rewind_ctx.capture_busy || !rewind_ctx.free_stack  || !rewind_ctx.queue) {
		PA_ERROR("Rewind: failed to allocate async capture buffers\n");
		rewind_free();
		return;
	}
	for (int i = 0; i < rewind_ctx.pool_size; i++) {
		rewind_ctx.capture_pool[i] = calloc(1, state_size);
		if (!rewind_ctx.capture_pool[i]) {
			PA_ERROR("Rewind: failed to allocate capture slot %i\n", i);
			rewind_free();
			return;
		}
		rewind_ctx.free_stack[i] = i;
	}
	rewind_ctx.queue_capacity = rewind_ctx.pool_size;
	rewind_ctx.free_count     = rewind_ctx.pool_size;

	if (pthread_create(&rewind_ctx.worker, NULL, Rewind_worker_thread, NULL) != 0) {
		PA_ERROR("Rewind: failed to start worker thread, "
		         "falling back to synchronous capture\n");
		rewind_ctx.pool_size      = 0;
		rewind_ctx.queue_capacity = 0;
		rewind_ctx.free_count     = 0;
	} else {
		rewind_ctx.worker_running = 1;
	}

	PA_INFO("Rewind: enabled (%zu bytes buffer, cadence %i ms)\n",
	        rewind_ctx.capacity, rewind_ctx.interval_ms);
}

void rewind_push(int force)
{
	if (!rewind_ctx.enabled) return;
	if (!rewind_ctx.buffer || !rewind_ctx.state_buf) return;

	uint32_t now_ms = (uint32_t)plat_get_ticks_ms();
	if (!force) {
		if (rewind_ctx.use_time_cadence) {
			if (rewind_ctx.last_push_ms &&
			    (int)(now_ms - rewind_ctx.last_push_ms) < rewind_ctx.interval_ms)
				return;
			rewind_ctx.last_push_ms = now_ms;
		} else {
			rewind_ctx.frame_counter += 1;
			if (rewind_ctx.frame_counter < rewind_ctx.granularity_frames)
				return;
			rewind_ctx.frame_counter = 0;
		}
	} else {
		rewind_ctx.frame_counter = 0;
		rewind_ctx.last_push_ms  = now_ms;
	}

	if (!current_core.retro_serialize || !current_core.retro_serialize_size) return;

	/* ── async path ─── */
	if (rewind_ctx.worker_running && rewind_ctx.pool_size) {
		int slot = -1;

		while (1) {
			pthread_mutex_lock(&rewind_ctx.queue_mx);
			if (rewind_ctx.free_count &&
			    rewind_ctx.queue_count < rewind_ctx.queue_capacity) {
				slot = rewind_ctx.free_stack[--rewind_ctx.free_count];
				rewind_ctx.capture_busy[slot] = 1;
				pthread_mutex_unlock(&rewind_ctx.queue_mx);
				break;
			}
			/* drain oldest queued capture synchronously to preserve ordering */
			if (rewind_ctx.queue_count > 0) {
				int qs = rewind_ctx.queue[rewind_ctx.queue_head];
				unsigned int gen = rewind_ctx.capture_gen[qs];
				rewind_ctx.queue_head = (rewind_ctx.queue_head + 1) % rewind_ctx.queue_capacity;
				rewind_ctx.queue_count -= 1;
				pthread_mutex_unlock(&rewind_ctx.queue_mx);

				size_t dest_len = rewind_ctx.scratch_size;
				int is_kf = 1;
				pthread_mutex_lock(&rewind_ctx.lock);
				if (gen == rewind_ctx.generation) {
					int res = Rewind_compress_state(rewind_ctx.capture_pool[qs],
					                                &dest_len, &is_kf);
					if (res == 0)
						Rewind_write_entry_locked(rewind_ctx.scratch, dest_len, is_kf);
					else
						PA_ERROR("Rewind: compression failed (%i)\n", res);
				}
				pthread_mutex_unlock(&rewind_ctx.lock);

				pthread_mutex_lock(&rewind_ctx.queue_mx);
				rewind_ctx.capture_busy[qs] = 0;
				rewind_ctx.free_stack[rewind_ctx.free_count++] = qs;
				pthread_mutex_unlock(&rewind_ctx.queue_mx);
				continue;
			}
			pthread_mutex_unlock(&rewind_ctx.queue_mx);
			break;
		}

		if (slot < 0) {
			/* worker busy – synchronous fallback */
			if (!current_core.retro_serialize(rewind_ctx.state_buf, rewind_ctx.state_size)) {
				PA_ERROR("Rewind: serialize failed (sync fallback)\n");
				return;
			}
			size_t dest_len = rewind_ctx.scratch_size;
			int is_kf = 1;
			pthread_mutex_lock(&rewind_ctx.lock);
			if (Rewind_compress_state(rewind_ctx.state_buf, &dest_len, &is_kf) == 0)
				Rewind_write_entry_locked(rewind_ctx.scratch, dest_len, is_kf);
			else
				PA_ERROR("Rewind: compression failed (sync fallback)\n");
			pthread_mutex_unlock(&rewind_ctx.lock);
			return;
		}

		if (!current_core.retro_serialize(rewind_ctx.capture_pool[slot],
		                                  rewind_ctx.state_size)) {
			PA_ERROR("Rewind: serialize failed\n");
			pthread_mutex_lock(&rewind_ctx.queue_mx);
			rewind_ctx.capture_busy[slot] = 0;
			rewind_ctx.free_stack[rewind_ctx.free_count++] = slot;
			pthread_mutex_unlock(&rewind_ctx.queue_mx);
			return;
		}
		rewind_ctx.capture_gen[slot] = rewind_ctx.generation;
		pthread_mutex_lock(&rewind_ctx.queue_mx);
		rewind_ctx.queue[rewind_ctx.queue_tail] = slot;
		rewind_ctx.queue_tail = (rewind_ctx.queue_tail + 1) % rewind_ctx.queue_capacity;
		rewind_ctx.queue_count += 1;
		pthread_cond_signal(&rewind_ctx.queue_cv);
		pthread_mutex_unlock(&rewind_ctx.queue_mx);
		return;
	}

	/* ── synchronous fallback ─── */
	if (!current_core.retro_serialize(rewind_ctx.state_buf, rewind_ctx.state_size)) {
		PA_ERROR("Rewind: serialize failed\n");
		return;
	}
	size_t dest_len = rewind_ctx.scratch_size;
	int is_kf = 1;
	pthread_mutex_lock(&rewind_ctx.lock);
	if (Rewind_compress_state(rewind_ctx.state_buf, &dest_len, &is_kf) == 0)
		Rewind_write_entry_locked(rewind_ctx.scratch, dest_len, is_kf);
	else
		PA_ERROR("Rewind: compression failed\n");
	pthread_mutex_unlock(&rewind_ctx.lock);
}

int rewind_step_back(void)
{
	if (!rewind_ctx.enabled) return REWIND_STEP_EMPTY;

	uint32_t now_ms = (uint32_t)plat_get_ticks_ms();

	if (rewind_ctx.playback_interval_ms > 0 && rewind_ctx.last_step_ms &&
	    (int)(now_ms - rewind_ctx.last_step_ms) < rewind_ctx.playback_interval_ms)
		return REWIND_STEP_CADENCE;

	/* On first rewind step, sync decode reference to encode reference */
	if (!rewinding && rewind_ctx.compress && rewind_ctx.prev_state_dec) {
		Rewind_wait_for_worker_idle();
		pthread_mutex_lock(&rewind_ctx.lock);
		if (rewind_ctx.has_prev_enc && rewind_ctx.prev_state_enc) {
			memcpy(rewind_ctx.prev_state_dec, rewind_ctx.prev_state_enc,
			       rewind_ctx.state_size);
			rewind_ctx.has_prev_dec = 1;
		} else {
			rewind_ctx.has_prev_dec = 0;
		}
		pthread_mutex_unlock(&rewind_ctx.lock);
	}

	pthread_mutex_lock(&rewind_ctx.lock);

	if (Rewind_buffer_state_locked() == REWIND_BUF_EMPTY) {
		pthread_mutex_unlock(&rewind_ctx.lock);
		if (!rewind_warn_empty) {
			PA_INFO("Rewind: no buffered states yet\n");
			rewind_warn_empty = 1;
		}
		return REWIND_STEP_EMPTY;
	}

	int idx = rewind_ctx.entry_head - 1;
	if (idx < 0) idx += rewind_ctx.entry_capacity;
	RewindEntry *e = &rewind_ctx.entries[idx];

	int decode_ok = 1;
	if (rewind_ctx.compress) {
		int res = LZ4_decompress_safe(
			(const char *)(rewind_ctx.buffer + e->offset),
			(char *)rewind_ctx.delta_buf,
			(int)e->size,
			(int)rewind_ctx.state_size);
		if (res < (int)rewind_ctx.state_size) {
			PA_ERROR("Rewind: decompress failed (res=%i, want=%zu, "
			         "compressed=%zu, offset=%zu)\n",
			         res, rewind_ctx.state_size, e->size, e->offset);
			decode_ok = 0;
		} else if (e->is_keyframe) {
			memcpy(rewind_ctx.state_buf, rewind_ctx.delta_buf, rewind_ctx.state_size);
			if (rewind_ctx.prev_state_dec) {
				memcpy(rewind_ctx.prev_state_dec, rewind_ctx.state_buf,
				       rewind_ctx.state_size);
				rewind_ctx.has_prev_dec = 1;
			}
		} else if (rewind_ctx.has_prev_dec && rewind_ctx.prev_state_dec) {
			size_t sz = rewind_ctx.state_size;
			uint8_t *result     = rewind_ctx.state_buf;
			const uint8_t *delta = rewind_ctx.delta_buf;
			const uint8_t *prev  = rewind_ctx.prev_state_dec;
			for (size_t i = 0; i < sz; i++)
				result[i] = delta[i] ^ prev[i];
			memcpy(rewind_ctx.prev_state_dec, result, sz);
		} else {
			PA_WARN("Rewind: delta frame without previous state, "
			        "results may be incorrect\n");
			memcpy(rewind_ctx.state_buf, rewind_ctx.delta_buf, rewind_ctx.state_size);
			if (rewind_ctx.prev_state_dec) {
				memcpy(rewind_ctx.prev_state_dec, rewind_ctx.state_buf,
				       rewind_ctx.state_size);
				rewind_ctx.has_prev_dec = 1;
			}
		}
	} else {
		if (e->size != rewind_ctx.state_size) {
			PA_ERROR("Rewind: raw snapshot size mismatch "
			         "(got=%zu, want=%zu)\n", e->size, rewind_ctx.state_size);
			decode_ok = 0;
		} else {
			memcpy(rewind_ctx.state_buf,
			       rewind_ctx.buffer + e->offset,
			       rewind_ctx.state_size);
		}
	}

	if (!decode_ok) {
		/* drop the corrupted newest entry */
		rewind_ctx.entry_head  = idx;
		rewind_ctx.entry_count -= 1;
		if (rewind_ctx.entry_count == 0)
			rewind_ctx.head = rewind_ctx.tail = 0;
		pthread_mutex_unlock(&rewind_ctx.lock);
		return REWIND_STEP_EMPTY;
	}

	if (!current_core.retro_unserialize(rewind_ctx.state_buf, rewind_ctx.state_size)) {
		PA_ERROR("Rewind: unserialize failed\n");
		Rewind_drop_oldest_locked();
		pthread_mutex_unlock(&rewind_ctx.lock);
		return REWIND_STEP_EMPTY;
	}

	/* pop newest */
	rewind_ctx.entry_head  = idx;
	rewind_ctx.entry_count -= 1;
	if (rewind_ctx.entry_count == 0)
		rewind_ctx.head = rewind_ctx.tail = 0;

	pthread_mutex_unlock(&rewind_ctx.lock);

	rewinding               = 1;
	rewind_ctx.last_step_ms = now_ms;
	return REWIND_STEP_OK;
}

void rewind_sync_encode_state(void)
{
	if (!rewind_ctx.enabled || !rewind_ctx.compress) return;
	if (!rewinding) return;

	pthread_mutex_lock(&rewind_ctx.lock);
	if (rewind_ctx.has_prev_dec && rewind_ctx.prev_state_dec && rewind_ctx.prev_state_enc) {
		memcpy(rewind_ctx.prev_state_enc, rewind_ctx.prev_state_dec,
		       rewind_ctx.state_size);
		rewind_ctx.has_prev_enc = 1;
	} else {
		rewind_ctx.has_prev_enc = 0;
	}
	pthread_mutex_unlock(&rewind_ctx.lock);
}

void rewind_on_state_change(void)
{
	rewind_reset();
	rewind_push(1);
	PA_INFO("Rewind: state changed, buffer re-seeded\n");
}
