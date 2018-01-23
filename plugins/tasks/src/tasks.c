#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "plugin_config.h"

void init_config(struct config* config)
{
	plugin_init_config(config);
}

int config(const char* name, const char* value)
{
	return plugin_config(name, value);
}

short init_tag(struct tag* tag)
{
	return plugin_init_tag(tag);
}

static xcb_window_t* list_windows(struct properties* properties,
	int* size,
	xcb_get_property_reply_t** reply)
{
	xcb_get_property_cookie_t cookie;
	xcb_window_t* start;
	cookie = xcb_ewmh_get_client_list_stacking(properties->ewmh_conn,
			properties->screen_id);
	*reply = xcb_get_property_reply(properties->xcb_conn, cookie, NULL);
	start = xcb_get_property_value(*reply);
	*size = xcb_get_property_value_length(*reply) / (sizeof(xcb_window_t));
	return start;
}

static void make_tags(struct properties* properties,
	int thread_id,
	xcb_window_t* windows,
	int size,
	xcb_window_t** windows_visible,
	xcb_window_t* window_active)
{
	xcb_generic_error_t* error;
	int i;
	int j;
	int visible_size;
	struct tag* tag;
	// active workspace
	xcb_get_property_cookie_t workspace_cookie;
	xcb_get_property_reply_t* workspace_reply;
	uint8_t* workspace_id;
	// workspace of a window
	xcb_get_property_cookie_t window_workspace_cookie;
	xcb_get_property_reply_t* window_workspace_reply;
	uint8_t* window_workspace;
	// title of a window
	xcb_get_property_cookie_t title_cookie;
	xcb_get_property_reply_t* title_reply;
	xcb_ewmh_get_utf8_strings_reply_t title_data;
	uint8_t title_error;
	char* title;
	error = NULL;
	// gets current workspace number
	workspace_cookie = xcb_ewmh_get_current_desktop(properties->ewmh_conn,
			properties->screen_id);
	workspace_reply = xcb_get_property_reply(properties->xcb_conn,
			workspace_cookie, NULL);
	workspace_id = xcb_get_property_value(workspace_reply);

	// frees previous tags
	for(i = 0; i < properties->tags_size[thread_id]; ++i)
	{
		tag_free(properties->tags[thread_id] + i);
	}

	// allocates the tags list
	if(size != 0)
	{
		properties->tags[thread_id] = malloc(size * (sizeof(struct tag)));
		*windows_visible = malloc(size * (sizeof(xcb_window_t)));
	}

	// saves dereferences once the memory is allocated
	tag = properties->tags[thread_id];
	// gets the titles
	visible_size = 0;

	for(i = 0; i < size; ++i)
	{
		// gets window workspace
		window_workspace_cookie = xcb_ewmh_get_wm_desktop(properties->ewmh_conn,
				windows[i]);
		window_workspace_reply = xcb_get_property_reply(properties->xcb_conn,
				window_workspace_cookie,
				&error);

		if(error != NULL)
		{
			free(window_workspace_reply);
			continue;
		}

		window_workspace = xcb_get_property_value(window_workspace_reply);

		// skips window if not on current workspace
		if(*window_workspace != *workspace_id)
		{
			free(window_workspace_reply);
			continue;
		}

		free(window_workspace_reply);
		// gets the title
		title_cookie = xcb_ewmh_get_wm_name(properties->ewmh_conn,
				windows[i]);
		title_reply = xcb_get_property_reply(properties->xcb_conn,
				title_cookie,
				&error);

		if(error != NULL)
		{
			continue;
		}

		title_error = xcb_ewmh_get_utf8_strings_from_reply(
				properties->ewmh_conn,
				&title_data,
				title_reply);

		if(title_error == 0)
		{
			title = strdup("");
		}
		else
		{
			title = strndup(title_data.strings, title_data.strings_len);
		}

		// initializes the tag only after the critical steps
		init_tag(tag + visible_size);

		if(windows[i] == *window_active)
		{
			for(j = 0; j < 3; ++j)
			{
				(tag + visible_size)->color_background[j] = color_active[j];
			}
		}

		tag_set_strings(tag + visible_size, 0, NULL, title, NULL);
		(*windows_visible)[visible_size] = windows[i];
		++visible_size;
	}

	free(workspace_reply);
	properties->tags_size[thread_id] = visible_size;

	if(visible_size != 0)
	{
		properties->tags[thread_id] = realloc(properties->tags[thread_id],
				visible_size * (sizeof(struct tag)));
		*windows_visible = realloc(*windows_visible,
				visible_size * (sizeof(xcb_window_t)));
	}
}

static void map_clicked_window(struct properties* properties,
	short id,
	xcb_window_t* windows_visible,
	xcb_window_t* window_active)
{
	int i;
	int j;
	uint16_t click_pos_x;
	int tags_added_size;
	uint8_t button;
	button = ((xcb_button_press_event_t*)(properties->x_event))->detail;
	click_pos_x = ((xcb_button_press_event_t*)(
				properties->x_event))->event_x;
	tags_added_size = 0;
	i = 0;
	j = 0;

	while(i < properties->threads_total)
	{
		if(properties->tags[i] == NULL)
		{
			++i;
			continue;
		}

		tags_added_size += (properties->tags[i] + j)->width;

		if(tags_added_size < click_pos_x)
		{
			++j;

			if(j >= properties->tags_size[i])
			{
				j = 0;
				++i;
			}
		}
		else
		{
			break;
		}
	}

	// maps clicked window
	if(i == id)
	{
		if(button == 4 || button == 5)
		{
			for(i = 0; i < properties->tags_size[id]; ++i)
			{
				if(*window_active == windows_visible[i])
				{
					break;
				}
			}
		}

		if(button == 4)
		{
			xcb_ewmh_request_change_active_window(properties->ewmh_conn,
				properties->screen_id,
				windows_visible[i == 0 ? properties->tags_size[id] - 1 : i - 1],
				XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL,
				XCB_CURRENT_TIME,
				*window_active);
		}
		else if(button == 5)
		{
			xcb_ewmh_request_change_active_window(properties->ewmh_conn,
				properties->screen_id,
				windows_visible[i == properties->tags_size[id] - 1 ? 0 : i + 1],
				XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL,
				XCB_CURRENT_TIME,
				*window_active);
		}
		else
		{
			xcb_ewmh_request_change_active_window(properties->ewmh_conn,
				properties->screen_id,
				windows_visible[j],
				XCB_EWMH_CLIENT_SOURCE_TYPE_NORMAL,
				XCB_CURRENT_TIME,
				*window_active);
		}
	}
}

void* plugin(void* par)
{
	int id;
	struct properties* properties;
	// windows list
	xcb_get_property_reply_t* windows_reply;
	xcb_window_t* windows;
	xcb_window_t* windows_visible;
	int windows_size;
	// active window
	xcb_get_property_cookie_t window_active_cookie;
	xcb_get_property_reply_t* window_active_reply;
	xcb_window_t* window_active;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	init_config(&properties->config);
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 0;
	properties->tags[id] = NULL;
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);
	win_add_event(properties, XCB_CREATE_NOTIFY, id);
	win_add_event(properties, XCB_DESTROY_NOTIFY, id);
	win_add_event(properties, XCB_CONFIGURE_NOTIFY, id);
	win_add_event(properties, XCB_PROPERTY_NOTIFY, id);
	win_add_event(properties, XCB_BUTTON_PRESS, id);
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);

	while(1)
	{
		pthread_mutex_lock(&properties->mutexes_state[id + 1]);

		if(properties->state == 1)
		{
			break;
		}

		window_active_cookie = xcb_ewmh_get_active_window(properties->ewmh_conn,
				properties->screen_id);
		window_active_reply = xcb_get_property_reply(properties->xcb_conn,
				window_active_cookie,
				NULL);
		window_active = (xcb_window_t*) xcb_get_property_value(
				window_active_reply);

		if(properties->x_event_id == XCB_BUTTON_PRESS)
		{
			map_clicked_window(properties, id, windows_visible, window_active);
		}

		windows = list_windows(properties, &windows_size, &windows_reply);
		make_tags(properties, id, windows, windows_size, &windows_visible,
			window_active);
		free(windows_reply);
		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

	plugin_free();
	return NULL;
}
