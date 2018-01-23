#include <string.h>
#include "font.h"

void font_init(struct properties* properties)
{
	PangoFontDescription* font_desc;
	PangoLayout* font_layout;
	PangoWeight font_weight;

	if(strcmp(properties->config.font_weight, "bold") == 0)
	{
		font_weight = PANGO_WEIGHT_BOLD;
	}
	else
	{
		font_weight = PANGO_WEIGHT_NORMAL;
	}

	font_desc = pango_font_description_new();
	pango_font_description_set_family(font_desc,
		properties->config.font_name);
	pango_font_description_set_weight(font_desc, font_weight);
	pango_font_description_set_absolute_size(font_desc,
		properties->config.font_size * PANGO_SCALE);
	font_layout = pango_cairo_create_layout(properties->cairo_conn);
	pango_layout_set_font_description(font_layout, font_desc);
	pango_layout_set_height(font_layout,
		properties->win_height * PANGO_SCALE);
	pango_layout_set_ellipsize(font_layout, PANGO_ELLIPSIZE_END);
	properties->font_desc = font_desc;
	properties->font_layout = font_layout;
}

void font_free(struct properties* properties)
{
	g_object_unref(properties->font_layout);
	pango_font_description_free(properties->font_desc);
}
