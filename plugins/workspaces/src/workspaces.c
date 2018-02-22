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

static void show_clicked_workspace(struct properties* properties,
	uint32_t previous, uint32_t total)
{
	int workspace;

	switch(((xcb_button_press_event_t*)(properties->x_event))->detail)
	{
		case 4:
		{
			workspace = (previous == 0) ? total - 1 : previous - 1;
			break;
		}

		case 5:
		{
			workspace = (previous == total - 1) ? 0 : previous + 1;
			break;
		}

		default:
		{
			workspace = properties->click_subsection;
		}
	}

	xcb_ewmh_request_change_current_desktop(properties->ewmh_conn,
		properties->screen_id,
		workspace,
		XCB_CURRENT_TIME);
}

void* plugin(void* par)
{
	xcb_generic_error_t* xcb_error;
	int i;
	int j;
	int id;
	struct properties* properties;
	struct tag* tag;
	char workspace_string[3];
	xcb_get_property_cookie_t workspace_cookie;
	xcb_get_property_reply_t* workspace_reply;
	xcb_ewmh_get_utf8_strings_reply_t utf8_data;
	uint8_t workspace_error;
	uint32_t workspace_id;
	uint32_t workspaces;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	init_config(&properties->config);
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 0;
	properties->tags[id] = NULL;
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);
	win_add_event(properties, XCB_EXPOSE, id);
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

		// change workspace if the user clicked the plugin's tags
		if(properties->x_event_id == XCB_BUTTON_PRESS)
		{
			show_clicked_workspace(properties, workspace_id, workspaces);
		}

		// gets the number of workspaces
		workspace_cookie = xcb_ewmh_get_number_of_desktops(
				properties->ewmh_conn,
				properties->screen_id);
		workspace_reply = xcb_get_property_reply(properties->xcb_conn,
				workspace_cookie,
				&xcb_error);
		workspaces = *(uint32_t*)xcb_get_property_value(workspace_reply);
		free(workspace_reply);
		// gets the number of the active workspace (0-indexed)
		workspace_cookie = xcb_ewmh_get_current_desktop(properties->ewmh_conn,
				properties->screen_id);
		workspace_reply = xcb_get_property_reply(properties->xcb_conn,
				workspace_cookie,
				&xcb_error);
		workspace_id = *(uint32_t*)xcb_get_property_value(workspace_reply);
		free(workspace_reply);
		workspace_cookie = xcb_ewmh_get_desktop_names(properties->ewmh_conn,
				properties->screen_id);
		workspace_reply = xcb_get_property_reply(properties->xcb_conn,
				workspace_cookie,
				&xcb_error);
		workspace_error = xcb_ewmh_get_utf8_strings_from_reply(
				properties->ewmh_conn,
				&utf8_data,
				workspace_reply);
		// buids the tags stream
		properties->tags_size[id] = workspaces;
		properties->tags[id] = realloc(properties->tags[id],
				(properties->tags_size[id]) * (sizeof(struct tag)));
		tag = properties->tags[id];
		j = 0;

		for(i = 0; i < workspaces; ++i)
		{
			if(workspace_error == 0)
			{
				sprintf(workspace_string, "%hhu", i + 1);
			}
			else
			{
				strncpy(workspace_string, utf8_data.strings + j,
					utf8_data.strings_len - j);
				j += strlen(utf8_data.strings + j) + 1;
			}

			init_tag(tag + i);
			tag_set_strings(tag + i, 1, prefix, workspace_string, postfix);
		}

		for(i = 0; i < 3; ++i)
		{
			(tag + workspace_id)->color_background[i] = tag->color_active[i];
		}

		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

	plugin_free();
	return NULL;
}
