#pragma once

void devices_init(void);
void devices_destroy(void);
void devices_in_received_handler(DictionaryIterator *iter);
void devices_out_sent_handler(DictionaryIterator *sent);
void devices_out_failed_handler(DictionaryIterator *failed, AppMessageResult reason);
