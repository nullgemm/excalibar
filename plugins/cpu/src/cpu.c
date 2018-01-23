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
	plugin_config(name, value);
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
	struct properties* properties;
	struct tag* tag;
	FILE* proc;
	unsigned long long total_user;
	unsigned long long total_user_low;
	unsigned long long total_sys;
	unsigned long long total_idle;
	unsigned long long total_user_old;
	unsigned long long total_user_low_old;
	unsigned long long total_sys_old;
	unsigned long long total_idle_old;
	unsigned long long total;
	unsigned long long percent;
	char formatted[4];
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
	proc = fopen("/proc/stat", "r");

	if(proc == NULL)
	{
		goto cpu_setup;
	}

	fscanf(proc,
		"cpu %llu %llu %llu %llu",
		&total_user_old,
		&total_user_low_old,
		&total_sys_old,
		&total_idle_old);
	percent = 0;
	fclose(proc);
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

		proc = fopen("/proc/stat", "r");

		if(proc == NULL)
		{
			goto cpu_setup;
		}

		fscanf(proc,
			"cpu %llu %llu %llu %llu",
			&total_user,
			&total_user_low,
			&total_sys,
			&total_idle);
		fclose(proc);

		if(total_user >= total_user_old
			|| total_user_low >= total_user_low_old
			|| total_sys >= total_sys_old
			|| total_idle >= total_idle_old)
		{
			total = (total_user - total_user_old)
				+ (total_user_low - total_user_low_old)
				+ (total_sys - total_sys_old);
			percent = 100 * total;
			total += total_idle - total_idle_old;

			if(total != 0)
			{
				percent /= total;
			}
			else
			{
				percent = 0;
			}
		}

		snprintf(formatted, 4, "%3d", (int)(percent));
		total_user_old = total_user;
		total_user_low_old = total_user_low;
		total_sys_old = total_sys;
		total_idle_old = total_idle;
		tag_set_strings(tag, 1, prefix, formatted, postfix);
		pthread_mutex_unlock(&properties->mutexes_task[id + 1]);
	}

cpu_setup:
	plugin_free();
	return NULL;
}
