#include "common.h"
#include "data.h"
#include "proto.h"
#include "rewind.h"
#ifdef __PSP__
#include <pspctrl.h>
#endif

typedef struct {
	word size;
	byte data[REWIND_MAX_PAYLOAD];
} rewind_slot_t;

typedef struct {
	rewind_slot_t* slots;
	int capacity; // 0, 360, or 720
	int head;     // next write index (0 .. capacity - 1)
	int count;    // number of frames stored (0 .. capacity)
} rewind_ring_buffer_t;

static rewind_ring_buffer_t rewind_buf = {NULL, 0, 0, 0};
static int rewind_hold_ticks = 0;
static int rewind_speed_level = 2; // 2..5 arrows ("<<" up to "<<<<<")
static int prev_dpad_left = 0;
static int prev_dpad_right = 0;
static word last_rewind_drawn_room = 0;

static byte*  rewind_serialize_target = NULL;
static size_t rewind_serialize_offset = 0;
static size_t rewind_serialize_limit  = 0;

static int process_to_rewind_buffer(void* data, size_t data_size) {
	if (rewind_serialize_offset + data_size > REWIND_MAX_PAYLOAD) {
		return 0; // overflow guard
	}
	memcpy(rewind_serialize_target + rewind_serialize_offset, data, data_size);
	rewind_serialize_offset += data_size;
	return 1;
}

static int process_load_from_rewind_buffer(void* data, size_t data_size) {
	if (rewind_serialize_offset + data_size > rewind_serialize_limit) {
		return 0; // underflow guard
	}
	memcpy(data, rewind_serialize_target + rewind_serialize_offset, data_size);
	rewind_serialize_offset += data_size;
	return 1;
}

void rewind_set_mode(byte mode) {
	int new_capacity = 0;
	if (mode == REWIND_MODE_30SEC) {
		new_capacity = 360; // 30s @ 12 FPS
	} else if (mode == REWIND_MODE_60SEC) {
		new_capacity = 720; // 60s @ 12 FPS
	} else {
		new_capacity = 0;
	}

	if (new_capacity == 0) {
		rewind_free();
		rewind_mode = REWIND_MODE_OFF;
		return;
	}

	if (rewind_buf.capacity != new_capacity || rewind_buf.slots == NULL) {
		rewind_slot_t* new_slots = (rewind_slot_t*) realloc(rewind_buf.slots, new_capacity * sizeof(rewind_slot_t));
		if (new_slots == NULL) {
			rewind_free();
			rewind_mode = REWIND_MODE_OFF;
			return;
		}
		rewind_buf.slots = new_slots;
		rewind_buf.capacity = new_capacity;
		rewind_buf.head = 0;
		rewind_buf.count = 0;
	}
	rewind_mode = mode;
}

void rewind_init(byte mode) {
	rewind_set_mode(mode);
}

void rewind_free(void) {
	if (rewind_buf.slots != NULL) {
		free(rewind_buf.slots);
		rewind_buf.slots = NULL;
	}
	rewind_buf.capacity = 0;
	rewind_buf.head = 0;
	rewind_buf.count = 0;
	rewind_hold_ticks = 0;
	rewind_speed_level = 2;
	prev_dpad_left = 0;
	prev_dpad_right = 0;
	last_rewind_drawn_room = 0;
}

void rewind_clear(void) {
	rewind_buf.head = 0;
	rewind_buf.count = 0;
	rewind_hold_ticks = 0;
	rewind_speed_level = 2;
	prev_dpad_left = 0;
	prev_dpad_right = 0;
	last_rewind_drawn_room = 0;
}

void rewind_record_frame(void) {
	if (rewind_mode == REWIND_MODE_OFF || rewind_buf.slots == NULL || rewind_buf.capacity <= 0) {
		return;
	}

	rewind_slot_t* slot = &rewind_buf.slots[rewind_buf.head];
	rewind_serialize_target = slot->data;
	rewind_serialize_offset = 0;
	rewind_serialize_limit  = REWIND_MAX_PAYLOAD;

	if (quick_process(process_to_rewind_buffer)) {
		slot->size = (word)rewind_serialize_offset;
		rewind_buf.head = (rewind_buf.head + 1) % rewind_buf.capacity;
		if (rewind_buf.count < rewind_buf.capacity) {
			rewind_buf.count++;
		}
	}
}

bool rewind_is_held(void) {
	if (rewind_mode == REWIND_MODE_OFF) return false;
#ifdef __PSP__
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);
	if (pad.Buttons & PSP_CTRL_LTRIGGER) {
		return true;
	} else {
		// Physical L trigger is not held: clear any lingering held flag
		joy_button_states[JOYINPUT_LEFTSHOULDER] &= ~KEYSTATE_HELD;
	}
#endif
	if (joy_button_states[JOYINPUT_LEFTSHOULDER] & KEYSTATE_HELD) return true;
	if (key_states[SDL_SCANCODE_L] & KEYSTATE_HELD) return true;
	if (key_states[SDL_SCANCODE_R] & KEYSTATE_HELD) return true;
	return false;
}

void rewind_handle_speed_input(void) {
	rewind_hold_ticks++;

	int curr_dpad_left = 0;
	int curr_dpad_right = 0;

#ifdef __PSP__
	SceCtrlData pad;
	sceCtrlPeekBufferPositive(&pad, 1);
	curr_dpad_left = (pad.Buttons & PSP_CTRL_LEFT) != 0;
	curr_dpad_right = (pad.Buttons & PSP_CTRL_RIGHT) != 0;
#endif
	if (!curr_dpad_left) {
		curr_dpad_left = (joy_button_states[JOYINPUT_DPAD_LEFT] & KEYSTATE_HELD) != 0 ||
		                 (key_states[SDL_SCANCODE_LEFT] & KEYSTATE_HELD) != 0;
	}
	if (!curr_dpad_right) {
		curr_dpad_right = (joy_button_states[JOYINPUT_DPAD_RIGHT] & KEYSTATE_HELD) != 0 ||
		                  (key_states[SDL_SCANCODE_RIGHT] & KEYSTATE_HELD) != 0;
	}

	// Tap D-pad Left: shift up to next gear (up to 5 arrows "<<<<<")
	if (curr_dpad_left && !prev_dpad_left) {
		if (rewind_speed_level < 5) {
			rewind_speed_level++;
		}
	}
	// Tap D-pad Right: shift down to previous gear (down to 1 arrow "<")
	if (curr_dpad_right && !prev_dpad_right) {
		if (rewind_speed_level > 1) {
			rewind_speed_level--;
		}
	}
	prev_dpad_left = curr_dpad_left;
	prev_dpad_right = curr_dpad_right;
}

int rewind_get_speed_level(void) {
	return rewind_speed_level;
}

const char* rewind_get_banner_text(bool is_limit) {
	static char banner[32];
	int arrows = rewind_speed_level;
	if (arrows < 1) arrows = 1;
	if (arrows > 5) arrows = 5;
	int idx = 0;
	for (int i = 0; i < arrows; i++) {
		banner[idx++] = '<';
	}
	banner[idx++] = ' ';
	if (is_limit) {
		strcpy(banner + idx, "REWIND (LIMIT)");
	} else {
		strcpy(banner + idx, "REWIND");
	}
	return banner;
}

int rewind_step_backward(void) {
	if (rewind_mode == REWIND_MODE_OFF || rewind_buf.slots == NULL || rewind_buf.count <= 0) {
		return 0;
	}

	if (rewind_speed_level == 1) {
		// Half-speed precision mode: only step backward on even hold ticks
		if ((rewind_hold_ticks % 2) != 0) {
			return 1;
		}
	}

	// Determine step size based on gear:
	// Level 1 ("<"):      half-speed (1 frame per 2 ticks @ 30 FPS = ~1.25x forward speed)
	// Level 2 ("<<"):     step 1 (1 frame per tick @ 30 FPS = ~2.5x forward speed)
	// Level 3 ("<<<"):    step 2 (2 frames per tick @ 30 FPS = ~5.0x forward speed)
	// Level 4 ("<<<<"):   step 3 (3 frames per tick @ 30 FPS = ~7.5x forward speed)
	// Level 5 ("<<<<<"):  step 5 (5 frames per tick @ 30 FPS = ~12.5x forward speed)
	int step = 1;
	if (rewind_speed_level == 3) {
		step = 2;
	} else if (rewind_speed_level == 4) {
		step = 3;
	} else if (rewind_speed_level >= 5) {
		step = 5;
	}

	if (step > rewind_buf.count) {
		step = rewind_buf.count;
	}
	if (step <= 0) {
		return 0;
	}

	// Move head back by step
	rewind_buf.head = (rewind_buf.head - step + rewind_buf.capacity) % rewind_buf.capacity;
	rewind_buf.count -= step;

	rewind_slot_t* slot = &rewind_buf.slots[rewind_buf.head];
	rewind_serialize_target = slot->data;
	rewind_serialize_offset = 0;
	rewind_serialize_limit  = slot->size;

	stop_sounds();

	int ok = quick_process(process_load_from_rewind_buffer);
	if (!ok) return 0;

	// Invalidate feather fall if fix is not enabled
	if (!fixes->fix_quicksave_during_feather && is_feather_fall > 0) {
		is_feather_fall = 0;
	}

	// Always sync camera with Kid's room and reload room links immediately
	next_room = drawn_room = Kid.room;
	load_room_links();
	if (drawn_room != last_rewind_drawn_room) {
		if (custom->tbl_level_type[current_level]) {
			gen_palace_wall_colors();
		}
		last_rewind_drawn_room = drawn_room;
	}
	different_room = 0;
	need_full_redraw = 0;

	hitp_delta = guardhp_delta = 1;
	if (Guard.room != drawn_room) {
		Guard.direction = dir_56_none;
		guardhp_curr = 0;
	}

	loadkid_and_opp();

	// Clear any death message or level restart state
	text_time_total = text_time_remaining = 0;
	exit_room_timer = 0;
	is_restart_level = 0;

	return 1;
}

int rewind_get_hold_ticks(void) {
	return rewind_hold_ticks;
}

void rewind_reset_hold_ticks(void) {
	rewind_hold_ticks = 0;
	rewind_speed_level = 2; // reset back to 2 arrows for next rewind
	prev_dpad_left = 0;
	prev_dpad_right = 0;
	last_rewind_drawn_room = 0;
}

int rewind_get_count(void) {
	return rewind_buf.count;
}
