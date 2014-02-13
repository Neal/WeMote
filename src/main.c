#include <pebble.h>
#include "appmessage.h"
#include "windows/devices.h"

static void init(void) {
	appmessage_init();
	devices_init();
}

static void deinit(void) {
	devices_destroy();
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
