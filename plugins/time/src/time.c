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

void* plugin(void* par)
{
	int id;
	struct properties* properties;
	struct tag* tag;
	time_t timer;
	struct tm* tm_info;
	char ascii[20];
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
	win_add_event(properties, XCB_PROPERTY_NOTIFY, id);
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);

	while(1)
	{
		pthread_mutex_lock(&properties->mutexes_state[id + 1]);

		if(properties->state == 1)
		{
			break;
		}

		time(&timer);
		tm_info = localtime(&timer);
		strftime(ascii, 20, "%d/%m/%Y %H:%M:%S", tm_info);
		tag_set_strings(tag, 1, prefix, ascii, postfix);
		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

	plugin_free();
	return NULL;
}
