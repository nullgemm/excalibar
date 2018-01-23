#include <string.h>
#include <cairo/cairo-xcb.h>
#include "win.h"

// connects to the display server and opens ewmh interface
short win_init_xcb(struct properties* properties)
{
	short result;
	properties->ewmh_conn = malloc(sizeof(xcb_ewmh_connection_t));
	properties->xcb_conn = xcb_connect(NULL,
			&(properties->screen_id));
	properties->xcb_cookie = xcb_ewmh_init_atoms(properties->xcb_conn,
			properties->ewmh_conn);
	properties->screen = xcb_setup_roots_iterator(xcb_get_setup(
				properties->xcb_conn)).data;
	result = xcb_ewmh_init_atoms_replies(properties->ewmh_conn,
			properties->xcb_cookie,
			NULL);
	return (result == 1) ? 0 : -1;
}

// uses X11 event system to communicate with threads
void win_ping(struct properties* properties)
{
	xcb_property_notify_event_t* event;

	if(properties->window == 0)
	{
		return;
	}

	event = calloc(32, 1);
	event->response_type = XCB_PROPERTY_NOTIFY;
	xcb_send_event(properties->xcb_conn,
		0,
		properties->window,
		XCB_EVENT_MASK_EXPOSURE,
		(char*)event);
	xcb_flush(properties->xcb_conn);
	free(event);
}

// uses X11 event system to communicate with threads
void win_render(struct properties* properties)
{
	xcb_expose_event_t* event;

	if(properties->window == 0)
	{
		return;
	}

	event = calloc(32, 1);
	event->response_type = XCB_EXPOSE;
	xcb_send_event(properties->xcb_conn,
		0,
		properties->window,
		XCB_EVENT_MASK_EXPOSURE,
		(char*)event);
	xcb_flush(properties->xcb_conn);
	free(event);
}

// adds an event to the events array corresponding to thread_id
void win_add_event(struct properties* properties,
	uint8_t event_id,
	short thread_id)
{
	short array_size;

	// checks if the thread lacks an event array, then allocates it
	if(properties->plugins_events[thread_id] == NULL)
	{
		properties->plugins_events_size[thread_id] = 1;
		properties->plugins_events[thread_id] = malloc(sizeof(uint8_t));
		properties->plugins_events[thread_id][0] = event_id;
	}
	// otherwise just uses the existing one and appends the given event
	else
	{
		array_size = properties->plugins_events_size[thread_id];
		properties->plugins_events[thread_id] = realloc(
				properties->plugins_events[thread_id],
				(array_size + 1) * (sizeof(uint8_t)));
		properties->plugins_events[thread_id][array_size] = event_id;
		++(properties->plugins_events_size[thread_id]);
	}
}

// updates the root window's event masks so the main thread can
// receive an event on substructure updates and X property changes
static short win_update_root_events(struct properties* properties)
{
	xcb_generic_error_t* error;
	xcb_screen_t* screen;
	xcb_void_cookie_t cookie;
	uint32_t value;
	value = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE;
	screen = xcb_setup_roots_iterator(xcb_get_setup(
				properties->xcb_conn)).data;
	cookie = xcb_change_window_attributes_checked(properties->xcb_conn,
			screen->root,
			XCB_CW_EVENT_MASK,
			&value);
	error = xcb_request_check(properties->xcb_conn, cookie);

	if(error != NULL)
	{
		return -1;
	}

	return 0;
}

// gets i3's "root" window containing all the user's windows
// used when option "root_win" is set to "i3" in excalibar.cfg
static xcb_window_t win_get_i3_root(struct properties* properties)
{
	xcb_generic_error_t* error;
	uint16_t i;
	xcb_connection_t* xcb_conn;
	xcb_screen_t* screen;
	xcb_window_t window;
	// children list
	xcb_query_tree_cookie_t cookie_tree;
	xcb_query_tree_reply_t* reply_tree;
	xcb_window_t* tree;
	uint16_t tree_size;
	// name
	xcb_get_property_cookie_t cookie_name;
	xcb_get_property_reply_t* reply_name;
	int name_len;
	xcb_conn = properties->xcb_conn;
	window = XCB_NONE;
	// gets the root window children tree
	screen = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn)).data;
	cookie_tree = xcb_query_tree(xcb_conn, screen->root);
	reply_tree = xcb_query_tree_reply(xcb_conn, cookie_tree, &error);

	if(error != NULL)
	{
		return XCB_NONE;
	}

	tree = xcb_query_tree_children(reply_tree);
	tree_size = reply_tree->children_len;

	// processes the children list to find the window named "i3"
	for(i = 0; i < tree_size; ++i)
	{
		// gets the child's name
		cookie_name = xcb_ewmh_get_wm_name(properties->ewmh_conn, tree[i]);
		reply_name = xcb_get_property_reply(xcb_conn, cookie_name, &error);

		if(error != NULL)
		{
			return XCB_NONE;
		}

		name_len = xcb_get_property_value_length(reply_name);

		if((name_len != 0)
			&& (strncmp("i3",
					(char*)xcb_get_property_value(reply_name),
					name_len) == 0))
		{
			window = tree[i];
			free(reply_name);
			break;
		}

		free(reply_name);
	}

	free(reply_tree);
	return window;
}

static short win_restack(struct properties* properties)
{
	uint32_t mask;
	uint32_t values[2];
	mask = XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE;

	if(strcmp(properties->config.bar_root_win, "i3") == 0)
	{
		values[0] = win_get_i3_root(properties);

		if(values[0] == XCB_NONE)
		{
			values[0] = properties->screen->root;
		}
	}
	else
	{
		values[0] = properties->screen->root;
	}

	values[1] = XCB_STACK_MODE_ABOVE;

	if(values[0] != XCB_NONE)
	{
		xcb_configure_window_checked(properties->xcb_conn,
			properties->window,
			mask,
			values);
	}
	else
	{
		return -1;
	}

	return 0;
}

static void win_set_strut(struct properties* properties)
{
	xcb_ewmh_wm_strut_partial_t strut = {0};
	xcb_ewmh_connection_t* ewmh_conn;
	ewmh_conn = properties->ewmh_conn;
	strut.top = properties->win_height;
	strut.top_start_x = 0;
	strut.top_end_x = properties->config.bar_width;
	xcb_ewmh_set_wm_strut(ewmh_conn,
		properties->window,
		0,
		0,
		properties->win_height,
		0);
	xcb_ewmh_set_wm_strut_partial(ewmh_conn,
		properties->window,
		strut);
}

static short win_set_attr(struct properties* properties)
{
	xcb_generic_error_t* error;
	short i;
	xcb_connection_t* xcb_conn = properties->xcb_conn;
	xcb_ewmh_connection_t* ewmh_conn = properties->ewmh_conn;
	xcb_window_t window = properties->window;
	xcb_intern_atom_cookie_t atom_cookie;
	xcb_intern_atom_reply_t* atom_reply;
	xcb_atom_t atoms[3];
	char* atoms_names[3] = {"_NET_WM_WINDOW_TYPE_DOCK",
			"_NET_WM_STATE_STICKY",
			"_NET_WM_STATE_ABOVE"
		};

	for(i = 0; i < 3; ++i)
	{
		atom_cookie = xcb_intern_atom(xcb_conn,
				0,
				strlen(atoms_names[i]),
				atoms_names[i]);
		atom_reply = xcb_intern_atom_reply(xcb_conn,
				atom_cookie,
				&error);

		if(error != NULL)
		{
			return -1;
		}

		atoms[i] = atom_reply->atom;
		free(atom_reply);
	}

	xcb_ewmh_set_wm_window_type(ewmh_conn, window, 1, &atoms[0]);
	xcb_ewmh_set_wm_state(ewmh_conn, window, 2, &atoms[1]);
	xcb_ewmh_set_wm_desktop(ewmh_conn, window, -1);
	return 0;
}

static xcb_visualtype_t* win_get_visual_type(struct properties*
	properties)
{
	xcb_visualtype_t* visual_type;
	xcb_screen_t* screen;
	xcb_depth_iterator_t depth_iter;
	visual_type = NULL;
	screen = properties->screen;
	depth_iter = xcb_screen_allowed_depths_iterator(screen);
	xcb_visualtype_iterator_t visual_iter;

	while(depth_iter.rem && visual_type == NULL)
	{
		visual_iter = xcb_depth_visuals_iterator(depth_iter.data);

		while(visual_iter.rem && visual_type == NULL)
		{
			if(screen->root_visual == visual_iter.data->visual_id)
			{
				visual_type = visual_iter.data;
				break;
			}

			xcb_visualtype_next(&visual_iter);
		}

		xcb_depth_next(&depth_iter);
	}

	return visual_type;
}

short win_create(struct properties* properties)
{
	xcb_connection_t* xcb_conn;
	xcb_drawable_t window;
	xcb_screen_t* screen;
	xcb_visualtype_t* visual_type;
	cairo_t* cairo_conn;
	cairo_surface_t* surface;
	uint32_t mask;
	uint32_t value;
	xcb_conn = properties->xcb_conn;
	mask = XCB_CW_EVENT_MASK;
	value = XCB_EVENT_MASK_EXPOSURE
		| XCB_EVENT_MASK_VISIBILITY_CHANGE
		| XCB_EVENT_MASK_BUTTON_PRESS
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_FOCUS_CHANGE;
	// creates the window
	window = xcb_generate_id(xcb_conn);
	screen = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn)).data;
	// X-plow-zean
	xcb_create_window(xcb_conn,
		XCB_COPY_FROM_PARENT,
		window,
		screen->root,
		0,
		0,
		properties->config.bar_width,
		properties->config.bar_height,
		0,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		screen->root_visual,
		mask,
		&value);
	// saves the structures
	properties->window = window;
	properties->screen = screen;
	// configures the window
	win_update_root_events(properties);
	win_set_attr(properties);
	win_set_strut(properties);
	win_restack(properties);
	// maps and starts cairo
	xcb_map_window(xcb_conn, window);
	visual_type = win_get_visual_type(properties);
	surface = cairo_xcb_surface_create(xcb_conn,
			window,
			visual_type,
			properties->config.bar_width,
			properties->config.bar_height);
	cairo_conn = cairo_create(surface);
	cairo_surface_flush(surface);
	xcb_flush(xcb_conn);
	// saves the structures
	properties->cairo_conn = cairo_conn;
	properties->cairo_surface = surface;
	return 0;
}
