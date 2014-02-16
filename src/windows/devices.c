#include <pebble.h>
#include "devices.h"
#include "../libs/pebble-assist.h"
#include "../common.h"

#define MAX_DEVICES 30

static device_t devices[MAX_DEVICES];

static int num_devices = 0;
static bool no_server = false;
static bool no_devices = false;
static bool out_failed = false;
static bool conn_timeout = false;
static bool conn_error = false;
static bool server_error = false;

static void refresh();
static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window;
static MenuLayer *menu_layer;

void devices_init(void) {
	window = window_create();

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	window_stack_push(window, true);
}

void devices_destroy(void) {
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void devices_in_received_handler(DictionaryIterator *iter) {
	Tuple *index_tuple = dict_find(iter, KEY_INDEX);
	Tuple *name_tuple = dict_find(iter, KEY_NAME);
	Tuple *host_tuple = dict_find(iter, KEY_HOST);
	Tuple *type_tuple = dict_find(iter, KEY_TYPE);
	Tuple *state_tuple = dict_find(iter, KEY_STATE);
	Tuple *error_tuple = dict_find(iter, KEY_ERROR);

	if (error_tuple) {
		if (strcmp(error_tuple->value->cstring, "no_server_set") != 0) {
			no_server = true;
		} else if (strcmp(error_tuple->value->cstring, "timeout") != 0) {
			conn_timeout = true;
		} else if (strcmp(error_tuple->value->cstring, "error") != 0) {
			conn_error = true;
		} else if (strcmp(error_tuple->value->cstring, "server_error") != 0) {
			server_error = true;
		}
		menu_layer_reload_data_and_mark_dirty(menu_layer);
	}
	else if (index_tuple && name_tuple && host_tuple && type_tuple && state_tuple) {
		no_server = false;
		out_failed = false;
		conn_timeout = false;
		conn_error = false;
		server_error = false;
		device_t device;
		device.index = index_tuple->value->int16;
		strncpy(device.name, name_tuple->value->cstring, sizeof(device.name) - 1);
		strncpy(device.host, host_tuple->value->cstring, sizeof(device.host) - 1);
		strncpy(device.type, type_tuple->value->cstring, sizeof(device.type) - 1);
		strncpy(device.state, state_tuple->value->cstring, sizeof(device.state) - 1);
		devices[device.index] = device;
	}
	else if (index_tuple) {
		num_devices = index_tuple->value->int16;
		no_devices = num_devices == 0;
		menu_layer_reload_data_and_mark_dirty(menu_layer);
	}
}

void devices_out_sent_handler(DictionaryIterator *sent) {
}

void devices_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason) {
	out_failed = true;
	menu_layer_reload_data_and_mark_dirty(menu_layer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh() {
	memset(devices, 0x0, sizeof(devices));
	num_devices = 0;
	no_server = false;
	no_devices = false;
	out_failed = false;
	conn_timeout = false;
	conn_error = false;
	server_error = false;
	menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
	app_message_outbox_send();
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return num_devices ? num_devices : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (out_failed || no_server || no_devices || conn_timeout || conn_error || num_devices == 0) {
		return 36;
	}
	return 50;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	menu_cell_basic_header_draw(ctx, cell_layer, "WeMote");
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	graphics_context_set_text_color(ctx, GColorBlack);
	if (out_failed) {
		graphics_draw_text(ctx, "Phone unreachable!", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (no_server) {
		graphics_draw_text(ctx, "No server set.", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (no_devices) {
		graphics_draw_text(ctx, "No devices found.", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (conn_timeout) {
		graphics_draw_text(ctx, "Connection timed out!", fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 44 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (conn_error) {
		graphics_draw_text(ctx, "HTTP Error!", fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 44 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (server_error) {
		graphics_draw_text(ctx, "Ouimeaux error!", fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 44 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else if (num_devices == 0) {
		graphics_draw_text(ctx, "Loading devices...", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), (GRect) { .origin = { 4, 4 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	} else {
		graphics_draw_text(ctx, devices[cell_index->row].name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), (GRect) { .origin = { 4, 2 }, .size = { 100, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, devices[cell_index->row].host, fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 24 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_draw_text(ctx, devices[cell_index->row].type, fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 24 }, .size = { PEBBLE_WIDTH - 8, 22 } }, GTextOverflowModeFill, GTextAlignmentRight, NULL);
		graphics_draw_text(ctx, devices[cell_index->row].state, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), (GRect) { .origin = { 106, -3 }, .size = { 34, 26 } }, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_devices == 0) {
		return;
	}
	strncpy(devices[cell_index->row].host, "Toggling...", sizeof(devices[cell_index->row].host) - 1);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
	Tuplet index_tuple = TupletInteger(KEY_INDEX, cell_index->row);
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	if (iter == NULL)
		return;
	dict_write_tuplet(iter, &index_tuple);
	dict_write_end(iter);
	app_message_outbox_send();
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	refresh();
}
