#include "draw.h"
#include "excalibar.h"
#include <xcb/xcb.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

static void draw_set_color_prefix(cairo_t* cairo_conn, struct tag* tag)
{
	cairo_set_source_rgb(cairo_conn,
		tag->color_prefix[0] / 255.0,
		tag->color_prefix[1] / 255.0,
		tag->color_prefix[2] / 255.0);
}

static void draw_set_color_postfix(cairo_t* cairo_conn, struct tag* tag)
{
	cairo_set_source_rgb(cairo_conn,
		tag->color_postfix[0] / 255.0,
		tag->color_postfix[1] / 255.0,
		tag->color_postfix[2] / 255.0);
}

static void draw_set_color_text(cairo_t* cairo_conn, struct tag* tag)
{
	cairo_set_source_rgb(cairo_conn,
		tag->color_text[0] / 255.0,
		tag->color_text[1] / 255.0,
		tag->color_text[2] / 255.0);
}

static void draw_set_color_line(cairo_t* cairo_conn, struct tag* tag)
{
	cairo_set_source_rgb(cairo_conn,
		tag->color_line[0] / 255.0,
		tag->color_line[1] / 255.0,
		tag->color_line[2] / 255.0);
}

static void draw_set_color_background(cairo_t* cairo_conn,
	struct tag* tag)
{
	cairo_set_source_rgb(cairo_conn,
		tag->color_background[0] / 255.0,
		tag->color_background[1] / 255.0,
		tag->color_background[2] / 255.0);
}

static int draw_txt(struct properties* properties,
	char* string,
	int max_len,
	short align,
	int x,
	int y)
{
	int width;
	int height;
	PangoLayout* font_layout;
	PangoAlignment text_alignment;
	cairo_t* cairo_conn;

	if(properties == NULL)
	{
		return 0;
	}

	if(string == NULL)
	{
		string = "";
	}

	switch(align)
	{
		case 2:
		{
			text_alignment = PANGO_ALIGN_CENTER;
			break;
		}

		case 1:
		{
			text_alignment = PANGO_ALIGN_RIGHT;
			break;
		}

		default:
		{
			text_alignment = PANGO_ALIGN_LEFT;
		}
	}

	font_layout = properties->font_layout;
	cairo_conn = properties->cairo_conn;
	// computes the y position with the height of the 't' character
	pango_layout_set_text(font_layout, "t", -1);
	y += (properties->win_height
			- (pango_layout_get_baseline(font_layout) / PANGO_SCALE)) / 2;
	// prints the text
	pango_layout_set_alignment(font_layout, text_alignment);
	pango_layout_set_text(font_layout, string, -1);
	pango_layout_set_width(font_layout, max_len * PANGO_SCALE);
	pango_layout_get_pixel_size(font_layout, &width, &height);
	cairo_move_to(cairo_conn, x, y);
	pango_cairo_show_layout(cairo_conn, font_layout);
	return width;
}

static int draw_get_y(struct properties* properties)
{
	int height;
	pango_layout_set_text(properties->font_layout, "t", -1);
	height = pango_layout_get_baseline(properties->font_layout)
		/ PANGO_SCALE;
	return (properties->win_height - height) / 2;
}

static void draw_tag(struct properties* properties,
	struct tag* tag,
	int max_len,
	int x,
	int y,
	int margin_left,
	int margin_right)
{
	int width;
	cairo_t* cairo_conn;
	cairo_conn = properties->cairo_conn;
	// draws the background rectangle
	draw_set_color_background(cairo_conn, tag);
	cairo_rectangle(cairo_conn, x, 0, max_len, properties->win_height);
	cairo_fill(cairo_conn);
	max_len -= margin_left + margin_right;
	x += margin_left;

	// draws the tag
	if(tag->align != 2)
	{
		y = tag->font_offset_top_prefix;
		pango_font_description_set_absolute_size(properties->font_desc,
			tag->font_size_prefix * PANGO_SCALE);
		pango_layout_set_font_description(properties->font_layout,
			properties->font_desc);
		draw_set_color_prefix(cairo_conn, tag);
		width = draw_txt(properties,
				tag->prefix,
				max_len,
				tag->align,
				x,
				y);
		x += width;
		max_len -= width;
	}

	y = tag->font_offset_top;
	pango_font_description_set_absolute_size(properties->font_desc,
		tag->font_size * PANGO_SCALE);
	pango_layout_set_font_description(properties->font_layout,
		properties->font_desc);
	draw_set_color_text(cairo_conn, tag);
	width = draw_txt(properties,
			tag->text,
			max_len,
			tag->align,
			x,
			y);

	if(tag->align != 2)
	{
		x += width;
		max_len -= width;
		y = tag->font_offset_top_postfix;
		pango_font_description_set_absolute_size(properties->font_desc,
			tag->font_size_postfix * PANGO_SCALE);
		pango_layout_set_font_description(properties->font_layout,
			properties->font_desc);
		draw_set_color_postfix(cairo_conn, tag);
		draw_txt(properties,
			tag->postfix,
			max_len,
			tag->align,
			x,
			y);
	}
}

static void draw_bounds(struct properties* properties, short max,
	short sign)
{
	short i;
	short j;
	short tag_init;
	short tag_stop;
	short arr_init;
	short arr_stop;
	int x;
	int y;
	int margin_left;
	int margin_right;
	struct tag* tag;
	int max_len;
	y = draw_get_y(properties);

	if(sign > 0)
	{
		x = 0;
		tag_init = 0;
		tag_stop = max;
	}
	else
	{
		x = properties->win_width;
		tag_init = max - 1;
		tag_stop = -1;
	}

	margin_left = properties->config.tag_margin_left;
	margin_right = properties->config.tag_margin_right;

	for(i = tag_init; i != tag_stop; i += sign)
	{
		tag = properties->tags[i];

		if((tag == NULL) || (1 - 2 * tag->side != sign))
		{
			continue;
		}

		if(sign > 0)
		{
			arr_init = 0;
			arr_stop = properties->tags_size[i];
		}
		else
		{
			arr_init = properties->tags_size[i] - 1;
			arr_stop = -1;
		}

		for(j = arr_init; j != arr_stop; j += sign)
		{
			max_len = tag->width;

			if(sign == -1)
			{
				x -= max_len;
			}

			draw_tag(properties, tag, max_len, x, y, margin_left, margin_right);

			if(sign == 1)
			{
				x += max_len;
			}

			tag += sign;
		}
	}
}

void draw(struct properties* properties)
{
	cairo_t* cairo_conn;
	cairo_conn = properties->cairo_conn;
	cairo_set_source_rgb(cairo_conn,
		properties->config.bar_color_background[0] / 255.0,
		properties->config.bar_color_background[1] / 255.0,
		properties->config.bar_color_background[2] / 255.0);
	cairo_paint(cairo_conn);
	draw_bounds(properties, properties->threads_total, 1);
	draw_bounds(properties, properties->threads_total, -1);
	cairo_surface_flush(properties->cairo_surface);
	xcb_flush(properties->xcb_conn);
}
