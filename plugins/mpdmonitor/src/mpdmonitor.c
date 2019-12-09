#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <mpd/client.h>
#include "plugin_config.h"

char* host;
unsigned int port;
unsigned int timeout;
unsigned int infolen;

void init_config(struct config* config)
{
	plugin_init_config(config);
	host = NULL;
	port = 0;
	timeout = 0;
	infolen = 50;
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
	else if(strcmp(name, "infolen") == 0)
	{
		infolen = abs(strtoul(value, NULL, 10));
		infolen = infolen < 10 ? 10 : infolen;
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
	int id;
	char* text;
	struct properties* properties;
	struct tag* tag;
	struct mpd_connection* conn;
	struct mpd_song* song;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	init_config(&properties->config);
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 1;
	properties->tags[id] = malloc((properties->tags_size[id]) * (sizeof(
					struct tag)));
	tag = properties->tags[id];
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);
	init_tag(tag);
	conn = mpd_connection_new(host, port, timeout);
	text = malloc(infolen * (sizeof(char)));
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);

	while(1)
	{
		if(properties->state == 1)
		{
			break;
		}

		if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
		{
			conn = mpd_connection_new(host, port, timeout);

			if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
			{
				sleep(5);
				continue;
			}
		}

		song = mpd_run_current_song(conn);

		if(song != NULL)
		{
			if(mpd_song_get_tag(song, MPD_TAG_TITLE, 0)) {
				snprintf(text, infolen, "%s - %s",
					mpd_song_get_tag(song, MPD_TAG_ARTIST, 0),
					mpd_song_get_tag(song, MPD_TAG_TITLE, 0));
			} else {
				snprintf(text, infolen, "%s",
					mpd_song_get_tag(song, MPD_TAG_NAME, 0));
			}
			mpd_song_free(song);
		}
		else
		{
			text[0] = '\0';
		}

		tag_set_strings(tag, 1, prefix, text, postfix);
		win_render(properties);
		mpd_run_idle_mask(conn, MPD_IDLE_PLAYER);
	}

	plugin_free();

	if(host != NULL)
	{
		free(host);
	}

	free(text);
	return NULL;
}
