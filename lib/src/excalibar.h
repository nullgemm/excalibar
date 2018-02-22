#ifndef EXCALIBAR_EXCALIBAR_H
#define EXCALIBAR_EXCALIBAR_H

#include "tag.h"
#include <pthread.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

// global configuration
struct config
{
	// general settings
	char* bar_root_win;
	int bar_width;
	int bar_height;
	// default tags values
	int tag_width;
	int tag_line_size;
	short tag_align;
	// general tags values
	short tag_width_unit; // 0=pixels, 1=chars (codepoints)
	short tag_margin_unit; // ditto
	int tag_margin_left;
	int tag_margin_right;
	int tag_offset_left;
	int tag_offset_right;
	// font
	char* font_name;
	int font_size;
	int font_size_prefix;
	int font_size_postfix;
	char* font_weight;
	int font_offset_top;
	int font_offset_top_prefix;
	int font_offset_top_postfix;
	// default colors
	uint8_t tag_color_prefix[3];
	uint8_t tag_color_postfix[3];
	uint8_t tag_color_text[3];
	uint8_t tag_color_line[3];
	uint8_t tag_color_active[3];
	uint8_t tag_color_background[3];
	uint8_t bar_color_background[3];
};

struct properties
{
	struct config config;
	// plugins threads
	pthread_mutex_t* mutexes_state;
	pthread_mutex_t* mutexes_task;
	pthread_t* threads;
	short threads_total;
	short threads_loaded;
	short state;
	// display server
	xcb_intern_atom_cookie_t* xcb_cookie;
	xcb_connection_t* xcb_conn;
	xcb_ewmh_connection_t* ewmh_conn;
	xcb_screen_t* screen;
	int screen_id;
	// main thread X event
	xcb_generic_event_t* x_event;
	uint8_t x_event_id;
	// plugins events subscription array
	uint8_t** plugins_events;
	short* plugins_events_size;
	// graphics engine
	cairo_t* cairo_conn;
	cairo_surface_t* cairo_surface;
	xcb_window_t window;
	// font
	PangoLayout* font_layout;
	PangoFontDescription* font_desc;
	// main array of plugins tags array
	struct tag** tags;
	short* tags_size;
	short tags_left;
	// realtime window size
	int win_width;
	int win_height;
	int click_subsection;
};

#endif /*EXCALIBAR_EXCALIBAR_H*/
