#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "plugin_config.h"

char* text;
char* cmd;

void init_config(struct config* config)
{
	text = NULL;
	cmd = NULL;
	plugin_init_config(config);
}

int config(const char* name, const char* value)
{
	if(strcmp(name, "text") == 0)
	{
		text = strdup(value);
	}
	else if(strcmp(name, "cmd") == 0)
	{
		cmd = strdup(value);
	}
	else
	{
		return plugin_config(name, value);
	}

	return 1;
}

short init_tag(struct tag* tag)
{
	return plugin_init_tag(tag);
}

void* plugin(void* par)
{
	int id;
	struct properties* properties;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	init_config(&properties->config);
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 1;
	properties->tags[id] = realloc(properties->tags[id],
			(properties->tags_size[id]) * (sizeof(struct tag)));
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);
	win_add_event(properties, XCB_EXPOSE, id);
	win_add_event(properties, XCB_BUTTON_PRESS, id);
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);
	init_tag(properties->tags[id]);
	tag_set_strings(properties->tags[id], 1, prefix, text, postfix);

	while(1)
	{
		pthread_mutex_lock(&properties->mutexes_state[id + 1]);

		if(properties->state == 1)
		{
			break;
		}

		if(properties->x_event_id == XCB_BUTTON_PRESS)
		{
			system(cmd);
		}

		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

	plugin_free();
	free(cmd);
	return NULL;
}
