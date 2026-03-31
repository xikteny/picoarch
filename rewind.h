#ifndef _REWIND_H__
#define _REWIND_H__

#include <stddef.h>

/* Return codes from rewind_step_back */
#define REWIND_STEP_EMPTY   0  /* buffer is empty or rewind is disabled */
#define REWIND_STEP_OK      1  /* stepped back one snapshot successfully */
#define REWIND_STEP_CADENCE 2  /* waiting for playback cadence; skip core, just re-render */

/* 1 while the user is holding the rewind button, 0 otherwise */
extern int rewinding;

void rewind_init(size_t state_size);
void rewind_free(void);
void rewind_reset(void);
void rewind_push(int force);
int  rewind_step_back(void);
void rewind_sync_encode_state(void);
void rewind_on_state_change(void);

#endif /* _REWIND_H__ */
