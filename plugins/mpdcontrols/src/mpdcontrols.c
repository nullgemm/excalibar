#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <mpd/client.h>
#include "plugin_config.h"

char* host;
unsigned int port;
unsigned int timeout;
char* mpd_prev;
char* mpd_next;
char* mpd_play;

void init_config(struct config* config)
{
	plugin_init_config(config);
	host = NULL;
	port = 0;
	timeout = 0;
	mpd_prev = strdup("⏮");
	mpd_next = strdup("⏭");
	mpd_play = strdup("⏯");
}

int config(const char* name, const char* value)
{
	plugin_config(name, value);

	if(strcmp(name, "host") == 0)
	{
		host = strdup(value);
	}
	else if(strcmp(name, "port") == 0)
	{
		port = abs(strtoul(value, NULL, 10));
	}
	else if(strcmp(name, "timeout") == 0)
	{
		timeout = abs(strtoul(value, NULL, 10));
	}
	else if(strcmp(name, "prev") == 0)
	{
		free(mpd_prev);
		mpd_prev = strdup(value);
	}
	else if(strcmp(name, "next") == 0)
	{
		free(mpd_next);
		mpd_next = strdup(value);
	}
	else if(strcmp(name, "play") == 0)
	{
		free(mpd_play);
		mpd_play = strdup(value);
	}

	return 1;
}

short init_tag(struct tag* tag)
{
	plugin_init_tag(tag);
	return 0;
}

void* plugin(void* par)
{
	int i;
	int id;
	struct properties* properties;
	struct tag* tag;
	struct mpd_connection* conn;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	init_config(&properties->config);
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 3;
	properties->tags[id] = malloc((properties->tags_size[id]) * (sizeof(
					struct tag)));
	tag = properties->tags[id];
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);

	for(i = 0; i < 3; ++i)
	{
		init_tag(tag + i);
	}

	tag_set_strings(tag, 1, NULL, mpd_prev, NULL);
	tag_set_strings(tag + 1, 1, NULL, mpd_next, NULL);
	tag_set_strings(tag + 2, 1, NULL, mpd_play, NULL);
	conn = mpd_connection_new(host, port, timeout);
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

		if(properties->x_event_id == XCB_BUTTON_PRESS)
		{
			if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
			{
				conn = mpd_connection_new(host, port, timeout);

				if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
				{
					pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
					continue;
				}
			}

			switch(properties->click_subsection)
			{
				case 0:
				{
					mpd_run_previous(conn);
					break;
				}

				case 1:
				{
					mpd_run_next(conn);
					break;
				}

				case 2:
				{
					mpd_run_toggle_pause(conn);
					break;
				}
			}
		}

		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

	plugin_free();

	if(host != NULL)
	{
		free(host);
	}

	free(mpd_prev);
	free(mpd_next);
	free(mpd_play);
	return NULL;
}
