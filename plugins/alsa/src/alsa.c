#include <excalibar/excalibar.h>
#include <excalibar/tag.h>
#include <excalibar/win.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <alsa/asoundlib.h>
#include "plugin_config.h"

char* card;
char* channel;

void init_config(struct config* config)
{
	plugin_init_config(config);
	card = strdup("default");
	channel = strdup("Master");
}

int config(const char* name, const char* value)
{
	plugin_config(name, value);

	if(strcmp(name, "card") == 0)
	{
		free(card);
		card = strdup(value);
	}
	else if(strcmp(name, "channel") == 0)
	{
		free(channel);
		channel = strdup(value);
	}

	return 1;
}

short init_tag(struct tag* tag)
{
	plugin_init_tag(tag);
	return 0;
}

static char* alsa_volume()
{
	int error;
	long min;
	long max;
	long level;
	snd_mixer_t* handle;
	snd_mixer_selem_id_t* sid;
	snd_mixer_elem_t* elem;
	static char text[4];
	strcpy(text, "  !");
	error = snd_mixer_open(&handle, 0);

	if(error < 0)
	{
		goto alsa_mixer_setup;
	}

	error = snd_mixer_attach(handle, card);

	if(error < 0)
	{
		goto alsa_mixer;
	}

	error = snd_mixer_selem_register(handle, NULL, NULL);

	if(error < 0)
	{
		goto alsa_mixer;
	}

	error = snd_mixer_load(handle);

	if(error < 0)
	{
		goto alsa_mixer;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, channel);
	elem = snd_mixer_find_selem(handle, sid);

	if(elem == NULL)
	{
		goto alsa_mixer;
	}

	error = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

	if(error < 0)
	{
		goto alsa_mixer;
	}

	error = snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO,
			&level);

	if(error < 0)
	{
		goto alsa_mixer;
	}

	snprintf(text, 4, "%3d", (int)(100 * (level - min) / max));
alsa_mixer:
	snd_mixer_close(handle);
alsa_mixer_setup:
	return text;
}

void* plugin(void* par)
{
	int error;
	int id;
	struct properties* properties;
	struct tag* tag;
	snd_ctl_t* ctl;
	snd_ctl_event_t* event;
	unsigned int mask;
	struct pollfd fd;
	unsigned short revents;
	char* level;
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
	// lets the main thread continue with another plugin
	pthread_mutex_unlock(&properties->mutexes_state[0]);
	error = snd_ctl_open(&ctl, card, SND_CTL_READONLY);

	if(error < 0)
	{
		goto alsa_setup;
	}

	error = snd_ctl_subscribe_events(ctl, 1);

	if(error < 0)
	{
		goto alsa_config;
	}

	level = alsa_volume();
	tag_set_strings(tag, 1, prefix, level, postfix);

	while(1)
	{
		error = snd_ctl_wait(ctl, 1000);

		if(properties->state == 1)
		{
			break;
		}

		if(error == 0)
		{
			continue;
		}

		snd_ctl_poll_descriptors(ctl, &fd, 1);
		error = poll(&fd, 1, -1);

		if(error <= 0)
		{
			break;
		}

		snd_ctl_poll_descriptors_revents(ctl, &fd, 1, &revents);

		if(revents & POLLIN)
		{
			snd_ctl_event_alloca(&event);
			error = snd_ctl_read(ctl, event);

			if(error < 0)
			{
				continue;
			}

			if(snd_ctl_event_get_type(event) != SND_CTL_EVENT_ELEM)
			{
				continue;
			}

			mask = snd_ctl_event_elem_get_mask(event);

			if(!(mask & SND_CTL_EVENT_MASK_VALUE))
			{
				continue;
			}

			level = alsa_volume();
			tag_set_strings(tag, 1, prefix, level, postfix);
			win_render(properties);
		}
	}

alsa_config:
	snd_ctl_close(ctl);
alsa_setup:
	plugin_free();
	free(card);
	free(channel);
	return NULL;
}
