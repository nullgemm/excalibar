#ifndef EXCALIBAR_WIN_H
#define EXCALIBAR_WIN_H

#include "excalibar.h"

short win_init_xcb(struct properties* properties);
void win_ping(struct properties* properties);
void win_render(struct properties* properties);
void win_add_event(struct properties* properties, uint8_t event_id,
	short thread_id);
short win_create(struct properties* properties);

#endif /*EXCALIBAR_WIN_H*/
