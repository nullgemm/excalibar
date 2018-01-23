#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int seconds;

int config(const char* name, const char* value)
{
	if(strcmp(name, "seconds") == 0)
	{
		seconds = strtoul(value, NULL, 10);
	}

	return 1;
}

void* plugin(void* par)
{
	int id;
	struct properties* properties;
	properties = (struct properties*)par;
	id = properties->threads_total - 1;
	seconds = 1;
	// lets the main thread load config
	pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	properties->tags_size[id] = 0;
	properties->tags[id] = NULL;
	// waits for the main thread to finish loading config
	pthread_mutex_lock(&properties->mutexes_state[id + 1]);
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);

	while(1)
	{
		if(properties->state == 1)
		{
			break;
		}

		sleep(seconds);
		win_ping(properties);
	}

	return NULL;
}
