#include "common.h"
#include "data.h"
#include "proto.h"
#include "rewind.h"

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
}

void rewind_clear(void) {
	rewind_buf.head = 0;
	rewind_buf.count = 0;
	rewind_hold_ticks = 0;
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
	// PSP L-Shoulder button:
	if (joy_button_states[JOYINPUT_LEFTSHOULDER] & KEYSTATE_HELD) return true;
	// Keyboard bindings (L or R):
	if (key_states[SDL_SCANCODE_L] & KEYSTATE_HELD) return true;
	if (key_states[SDL_SCANCODE_R] & KEYSTATE_HELD) return true;
	return false;
}

int rewind_step_backward(void) {
	if (rewind_mode == REWIND_MODE_OFF || rewind_buf.slots == NULL || rewind_buf.count <= 0) {
		return 0;
	}

	rewind_hold_ticks++;

	// Gentle acceleration: initial 4 ticks step 1 frame, then 2 frames per tick (2x speed)
	int step = (rewind_hold_ticks > 4) ? 2 : 1;
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

	// Restore room links, sprites, and HP without disk I/O
	different_room = 1;
	next_room = drawn_room = Kid.room;
	load_room_links();

	hitp_delta = guardhp_delta = 1;
	if (Guard.room != drawn_room) {
		Guard.direction = dir_56_none;
		guardhp_curr = 0;
	}

	draw_hp();
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
}

int rewind_get_count(void) {
	return rewind_buf.count;
}
