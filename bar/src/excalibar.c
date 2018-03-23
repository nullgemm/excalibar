// plugin loading sequence :
// basic init (alloc'ing required memory)
// waiting for the main thread to process the section entirely
// complete init (using the configuration values acquired)
// completion (unlocking the next plugin sequence)

#include <excalibar/tag.h>
#include <excalibar/excalibar.h>
#include <excalibar/win.h>
#include <excalibar/font.h>
#include <excalibar/draw.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cairo/cairo-xcb.h>
#include "ini.h"

#define EXCALIBAR_PLUGINS_PATH "plugins/"
#define EXCALIBAR_CONFIG_PATH "/.config/excalibar/"
#define EXCALIBAR_CONFIG_FILE "excalibar.cfg"

#define EXCALIBAR_ERR_HOME "Could not find home directory"
#define EXCALIBAR_ERR_CONFIG_FOLDER "Could not find the config folder. Running mkdir..."
#define EXCALIBAR_ERR_CONFIG_MKDIR "Could not create the config folder"
#define EXCALIBAR_ERR_CONFIG_FILE "Could not read the config file"
#define EXCALIBAR_ERR_CONFIG_READ "Could not get the parser to read the config file"
#define EXCALIBAR_ERR_CONFIG_INVALID "Invalid config file"
#define EXCALIBAR_ERR_PLUGINS_OVERDOSE "Loading too much plugins"

// global variable required for easy shutdown
struct properties properties;
void** handles;

// updates the state's width and height attributes
short geometry_update(struct properties* properties)
{
	xcb_generic_error_t* xcb_error;
	xcb_connection_t* xcb_conn;
	xcb_get_geometry_cookie_t cookie;
	xcb_get_geometry_reply_t* reply;
	xcb_conn = properties->xcb_conn;
	cookie = xcb_get_geometry(xcb_conn, properties->window);
	reply = xcb_get_geometry_reply(xcb_conn, cookie, &xcb_error);

	if(xcb_error != NULL)
	{
		return -1;
	}

	properties->win_width = reply->width;
	properties->win_height = reply->height;
	free(reply);
	return 0;
}

// launches the shutdown process
static void sigterm_handle(int signum)
{
	properties.state = 1;
	win_ping(&properties);
}

static char* get_home()
{
	char* home_path;
	struct passwd* passwd;
	home_path = getenv("HOME");

	if(home_path == NULL)
	{
		passwd = getpwuid(getuid());

		if(passwd == NULL)
		{
			return NULL;
		}

		home_path = passwd->pw_dir;
	}

	return home_path;
}

static int cursor_plugin(void)
{
	int i = 0;
	int j = 0;
	int tags_added_size = 0;
	short side_unchanged = 1;
	uint16_t click_pos_x = ((xcb_button_press_event_t*)
			(properties.x_event))->event_x;

	while(i < properties.threads_total)
	{
		if(properties.tags[i] == NULL)
		{
			++i;
			continue;
		}

		// first right tag
		if(side_unchanged == 1 && properties.tags[i]->side)
		{
			int k;
			int l;
			tags_added_size = properties.win_width;

			for(k = i; k < properties.threads_total; ++k)
			{
				if(properties.tags[k] != NULL)
				{
					for(l = 0; l < properties.tags_size[k]; ++l)
					{
						tags_added_size -= (properties.tags[k] + l)->width;
					}
				}
			}

			++side_unchanged;
		}

		// gap clicking
		if(side_unchanged == 2 && tags_added_size > click_pos_x)
		{
			++side_unchanged;
		}

		// builds current tag position
		tags_added_size += (properties.tags[i] + j)->width;

		// clicked after current tag
		if(tags_added_size < click_pos_x || side_unchanged == 3)
		{
			// if side_unchanged == 3 nothing happens,
			// so disabling the variable is not needed
			++j;

			if(j >= properties.tags_size[i])
			{
				j = 0;
				++i;
			}
		}
		// clicked on the current tag
		else
		{
			break;
		}
	}

	properties.click_subsection = j;
	return i < properties.threads_total ? i : -1;
}

static int load_config(void* user,
	const char* section,
	const char* name,
	const char* value)
{
	int tmp;
	int value_len;
	char* dyn_value;
	static int max_plugins = 20;
	static char last_section[50] = {0};
	char config_path[1024];
	char* home_path;
	void* (*plugin_function)(void*);
	int (*config_function)(const char*, const char*);
	value_len = strlen(value);

	if(value_len > 1 && value[0] == '\"' && value[value_len - 1] == '\"')
	{
		dyn_value = strdup(value + 1);
		dyn_value[value_len - 2] = 0;
	}
	else
	{
		dyn_value = strdup(value);
	}

	if((strcmp(section, "excalibar") == 0) && (*last_section == 0))
	{
		if(strcmp(name, "bar_root_win") == 0)
		{
			properties.config.bar_root_win = strdup(dyn_value);
		}
		else if(strcmp(name, "bar_width") == 0)
		{
			properties.config.bar_width = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "bar_height") == 0)
		{
			properties.config.bar_height = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_width") == 0)
		{
			properties.config.tag_width = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_line_size") == 0)
		{
			properties.config.tag_line_size = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_align") == 0)
		{
			properties.config.tag_align = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_line_size") == 0)
		{
			properties.config.tag_line_size = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_width_unit") == 0)
		{
			properties.config.tag_width_unit = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_margin_unit") == 0)
		{
			properties.config.tag_margin_unit = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_margin_left") == 0)
		{
			properties.config.tag_margin_left = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_margin_right") == 0)
		{
			properties.config.tag_margin_right = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_offset_left") == 0)
		{
			properties.config.tag_offset_left = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "tag_offset_right") == 0)
		{
			properties.config.tag_offset_right = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_name") == 0)
		{
			properties.config.font_name = strdup(dyn_value);
		}
		else if(strcmp(name, "font_size") == 0)
		{
			properties.config.font_size = strtoul(dyn_value, NULL, 10);
			properties.config.font_size_prefix = strtoul(dyn_value, NULL, 10);
			properties.config.font_size_postfix = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_size_prefix") == 0)
		{
			properties.config.font_size_prefix = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_size_postfix") == 0)
		{
			properties.config.font_size_postfix = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_weight") == 0)
		{
			properties.config.font_weight = strdup(dyn_value);
		}
		else if(strcmp(name, "font_offset_top") == 0)
		{
			properties.config.font_offset_top = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_offset_top_prefix") == 0)
		{
			properties.config.font_offset_top_prefix = strtoul(dyn_value, NULL, 10);
		}
		else if(strcmp(name, "font_offset_top_postfix") == 0)
		{
			properties.config.font_offset_top_postfix = strtoul(dyn_value,
					NULL,
					10);
		}
		else if(strcmp(name, "tag_color_prefix") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_prefix[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_prefix[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_prefix[2] = tmp & 0xff;
		}
		else if(strcmp(name, "tag_color_postfix") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_postfix[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_postfix[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_postfix[2] = tmp & 0xff;
		}
		else if(strcmp(name, "tag_color_text") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_text[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_text[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_text[2] = tmp & 0xff;
		}
		else if(strcmp(name, "tag_color_line") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_line[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_line[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_line[2] = tmp & 0xff;
		}
		else if(strcmp(name, "tag_color_background") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_background[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_background[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_background[2] = tmp & 0xff;
		}
		else if(strcmp(name, "tag_color_active") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.tag_color_active[0] = (tmp >> 16) & 0xff;
			properties.config.tag_color_active[1] = (tmp >> 8) & 0xff;
			properties.config.tag_color_active[2] = tmp & 0xff;
		}
		else if(strcmp(name, "bar_color_background") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 16);
			properties.config.bar_color_background[0] = (tmp >> 16) & 0xff;
			properties.config.bar_color_background[1] = (tmp >> 8) & 0xff;
			properties.config.bar_color_background[2] = tmp & 0xff;
		}
		else if(strcmp(name, "bar_max_plugins") == 0)
		{
			tmp = strtoul(dyn_value, NULL, 10);
			tmp = tmp > 50 ? 50 : tmp;
			tmp = tmp < 0 ? 20 : tmp;
			max_plugins = tmp;
			++tmp;
			properties.mutexes_state = realloc(properties.mutexes_state,
					tmp * (sizeof(pthread_mutex_t)));
			properties.mutexes_task = realloc(properties.mutexes_task,
					tmp * (sizeof(pthread_mutex_t)));
		}
	}
	else
	{
		// last_section is empty after the "excalibar" section so we can easily detect
		// when it is the first section and prevent from changing config later
		if(strcmp(last_section, section) != 0)
		{
			++properties.threads_total;

			if(properties.threads_total > max_plugins)
			{
				fprintf(stderr, "%s\n", EXCALIBAR_ERR_PLUGINS_OVERDOSE);
				return 0;
			}

			home_path = get_home();

			if(home_path == NULL)
			{
				return 0;
			}

			strcpy(config_path, home_path);
			strcat(config_path, EXCALIBAR_CONFIG_PATH);
			strcat(config_path, EXCALIBAR_PLUGINS_PATH);
			strcat(config_path, section);

			// allows the previous module to finish its initialization
			// once we loaded its config entirely
			if(properties.threads_total > 1)
			{
				pthread_mutex_unlock(&properties.mutexes_state
					[properties.threads_total - 1]);
			}

			// prevents us from loading 2 plugins at the same time
			pthread_mutex_lock(&properties.mutexes_state[0]);
			// resizes...
			handles = realloc(handles,
					properties.threads_total * (sizeof(void*)));
			properties.threads = realloc(properties.threads,
					properties.threads_total * (sizeof(void*)));
			properties.tags = realloc(properties.tags,
					properties.threads_total * (sizeof(struct tag*)));
			properties.tags_size = realloc(properties.tags_size,
					properties.threads_total * (sizeof(short)));
			properties.plugins_events = realloc(properties.plugins_events,
					properties.threads_total * (sizeof(uint8_t*)));
			properties.plugins_events_size = realloc(properties.plugins_events_size,
					properties.threads_total * (sizeof(short)));
			// secures
			properties.tags_size[properties.threads_total - 1] = 0;
			properties.plugins_events_size[properties.threads_total - 1] = 0;
			properties.plugins_events[properties.threads_total - 1] = NULL;
			pthread_mutex_init(&properties.mutexes_state[properties.threads_total],
				NULL);
			pthread_mutex_init(&properties.mutexes_task[properties.threads_total],
				NULL);
			// prevents the plugin from finishing initialization
			// before its config was entirely loaded
			pthread_mutex_lock(&properties.mutexes_state[properties.threads_total]);
			handles[properties.threads_total - 1] = dlopen(config_path, RTLD_LAZY);

			if(dlerror() != NULL)
			{
				return 0;
			}

			// prevents us from loading the plugin's config before it started initializing
			pthread_mutex_lock(&properties.mutexes_task[properties.threads_total]);
			*(void**)(&plugin_function) =
				dlsym(handles[properties.threads_total - 1],
					"plugin");
			// starts the plugin
			pthread_create(&properties.threads[properties.threads_total - 1],
				NULL,
				plugin_function,
				&properties);
		}

		// associates with the lock above
		pthread_mutex_lock(&properties.mutexes_task[properties.threads_total]);
		*(void**)(&config_function) =
			dlsym(handles[properties.threads_total - 1],
				"config");
		pthread_mutex_unlock(&properties.mutexes_task
			[properties.threads_total]);
		tmp = (*config_function)(name, dyn_value);

		if(tmp == 0)
		{
			return 0;
		}

		strcpy(last_section, section);
	}

	free(dyn_value);
	return 1;
}

static void config_default_strings()
{
	if(properties.config.bar_root_win == NULL)
	{
		properties.config.bar_root_win = strdup("default");
	}

	if(properties.config.font_name == NULL)
	{
		properties.config.font_name = strdup("Sans");
	}

	if(properties.config.font_weight == NULL)
	{
		properties.config.font_weight = strdup("normal");
	}
}

int main(void)
{
	int i;
	int j;
	int error;
	char* home_path;
	char config_path[1024];
	struct sigaction sigterm;
	handles = NULL;
	memset(&properties, 0, sizeof(properties));
	sigterm.sa_handler = sigterm_handle;
	sigterm.sa_flags = 0;
	sigemptyset(&sigterm.sa_mask);
	sigaddset(&sigterm.sa_mask, SIGTERM);
	sigaction(SIGTERM, &sigterm, NULL);
	home_path = get_home();

	if(home_path == NULL)
	{
		fprintf(stderr, "%s\n", EXCALIBAR_ERR_HOME);
		goto main_error;
	}

	strcpy(config_path, home_path);
	strcat(config_path, EXCALIBAR_CONFIG_PATH);

	// config folder not found
	if(access(config_path, R_OK) == -1)
	{
		fprintf(stderr, "%s\n", EXCALIBAR_ERR_CONFIG_FOLDER);
		error = mkdir(config_path,
				S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

		// could not create config folder
		if(error == -1)
		{
			fprintf(stderr, "%s\n", EXCALIBAR_ERR_CONFIG_MKDIR);
		}

		goto main_error;
	}

	strcat(config_path, EXCALIBAR_CONFIG_FILE);

	// folder exists but config file not found
	if(access(config_path, R_OK) == -1)
	{
		fprintf(stderr, "%s\n", EXCALIBAR_ERR_CONFIG_FILE);
		goto main_error;
	}

	properties.threads_total = 0;
	properties.mutexes_state = malloc(20 * (sizeof(pthread_mutex_t)));
	properties.mutexes_task = malloc(20 * (sizeof(pthread_mutex_t)));
	pthread_mutex_init(&properties.mutexes_state[0], NULL);
	// loads plugins and configuration from file
	error = ini_parse(config_path, load_config, NULL);

	// parser could not read file
	if(error < 0)
	{
		fprintf(stderr, "%s\n", EXCALIBAR_ERR_CONFIG_READ);
		printf("1\n");
		goto main_error;
	}

	// invalid config file
	if(error > 0)
	{
		fprintf(stderr, "%s\n", EXCALIBAR_ERR_CONFIG_INVALID);
		goto main_error;
	}

	// tells the last module we finished loading its config
	pthread_mutex_unlock(&properties.mutexes_state
		[properties.threads_total]);
	// loads default values for missing strings
	config_default_strings();
	// performs excalibar's initialisation
	win_init_xcb(&properties);
	win_create(&properties);
	font_init(&properties);

	while((properties.x_event = xcb_wait_for_event(properties.xcb_conn)))
	{
		pthread_mutex_lock(&properties.mutexes_state[0]);
		properties.x_event_id = properties.x_event->response_type & ~0x80;

		if(properties.x_event_id == XCB_BUTTON_PRESS)
		{
			i = cursor_plugin();

			if(i != -1)
			{
				for(j = 0; j < properties.plugins_events_size[i]; ++j)
				{
					if((properties.plugins_events[i][j] == properties.x_event_id)
						|| (properties.state == 1))
					{
						pthread_mutex_lock(&properties.mutexes_task[i + 1]);
						pthread_mutex_unlock(&properties.mutexes_state[i + 1]);
						break;
					}
				}
			}
		}
		else
		{
			for(i = 0; i < properties.threads_total; ++i)
			{
				if(properties.plugins_events[i] != NULL)
				{
					for(j = 0; j < properties.plugins_events_size[i]; ++j)
					{
						if((properties.plugins_events[i][j] == properties.x_event_id)
							|| (properties.state == 1))
						{
							pthread_mutex_lock(&properties.mutexes_task[i + 1]);
							pthread_mutex_unlock(&properties.mutexes_state[i + 1]);
							break;
						}
					}
				}
			}
		}

		if(properties.state == 1)
		{
			break;
		}

		for(i = 1; i <= properties.threads_total; ++i)
		{
			pthread_mutex_lock(&properties.mutexes_task[i]);
			pthread_mutex_unlock(&properties.mutexes_task[i]);
		}

		switch(properties.x_event_id)
		{
			case XCB_EXPOSE:
			case XCB_VISIBILITY_NOTIFY:
			case XCB_PROPERTY_NOTIFY:
			case XCB_CONFIGURE_NOTIFY:
			{
				geometry_update(&properties);
				cairo_xcb_surface_set_size(properties.cairo_surface,
					properties.win_width,
					properties.win_height);
				draw(&properties);
				break;
			}
		}

		free(properties.x_event);
		pthread_mutex_unlock(&properties.mutexes_state[0]);
	}

	// frees handles, tags
	for(i = 0; i < properties.threads_total; ++i)
	{
		pthread_join(properties.threads[i], NULL);
		dlclose(handles[i]);

		for(j = 0; j < properties.tags_size[i]; ++j)
		{
			tag_free(properties.tags[i] + j);
		}

		free(properties.tags[i]);
		free(properties.plugins_events[i]);
	}

	free(properties.x_event);
	// frees config strings
	free(properties.config.bar_root_win);
	free(properties.config.font_name);
	free(properties.config.font_weight);
	// frees properties
	free(properties.threads);
	font_free(&properties);
	cairo_surface_destroy(properties.cairo_surface);
	cairo_destroy(properties.cairo_conn);
	xcb_ewmh_connection_wipe(properties.ewmh_conn);
	xcb_disconnect(properties.xcb_conn);

	if(properties.plugins_events != NULL)
	{
		free(properties.plugins_events);
		free(properties.plugins_events_size);
	}

	if(properties.tags != NULL)
	{
		free(properties.tags_size);
	}

main_error:
	return 0;
}
