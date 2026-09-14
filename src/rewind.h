#ifndef REWIND_H
#define REWIND_H

#include "common.h"

#define REWIND_MAX_PAYLOAD 4096

// Modes: 0 = OFF, 1 = 30 SEC (360 frames), 2 = 60 SEC (720 frames)
enum {
	REWIND_MODE_OFF = 0,
	REWIND_MODE_30SEC = 1,
	REWIND_MODE_60SEC = 2,
};

void rewind_init(byte mode);
void rewind_set_mode(byte mode);
void rewind_clear(void);
void rewind_free(void);
void rewind_record_frame(void);
int  rewind_step_backward(void);
bool rewind_is_held(void);
int  rewind_get_hold_ticks(void);
void rewind_reset_hold_ticks(void);
int  rewind_get_count(void);
int  rewind_get_speed_level(void);
void rewind_handle_speed_input(void);
const char* rewind_get_banner_text(bool is_limit);

#endif // REWIND_H
