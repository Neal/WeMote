#pragma once

typedef struct {
	int index;
	char name[32];
	char host[32];
	char type[16];
	char state[4];
} device_t;

enum {
	KEY_INDEX = 0x0,
	KEY_NAME = 0x1,
	KEY_HOST = 0x2,
	KEY_TYPE = 0x3,
	KEY_STATE = 0x4,
};
