int tmp;
short side;
int width;
int line_size;
int font_size;
int font_size_prefix;
int font_size_postfix;
int font_offset_top;
int font_offset_top_prefix;
int font_offset_top_postfix;
short align;
uint8_t color_prefix[3];
uint8_t color_postfix[3];
uint8_t color_active[3];
uint8_t color_text[3];
uint8_t color_line[3];
uint8_t color_background[3];
char* prefix;
char* postfix;

void plugin_init_config(struct config* config)
{
	int i;
	side = 0;
	width = config->tag_width;
	line_size = config->tag_line_size;
	font_size = config->font_size;
	font_size_prefix = config->font_size_prefix;
	font_size_postfix = config->font_size_postfix;
	font_offset_top = config->font_offset_top;
	font_offset_top_prefix = config->font_offset_top_prefix;
	font_offset_top_postfix = config->font_offset_top_postfix;
	align = config->tag_align;
	prefix = NULL;
	postfix = NULL;

	for(i = 0; i < 3; ++i)
	{
		color_prefix[i] = config->tag_color_prefix[i];
		color_postfix[i] = config->tag_color_postfix[i];
		color_active[i] = config->tag_color_active[i];
		color_text[i] = config->tag_color_text[i];
		color_line[i] = config->tag_color_line[i];
		color_background[i] = config->tag_color_background[i];
	}
}

int plugin_config(const char* name, const char* value)
{
	if(strcmp(name, "side") == 0)
	{
		side = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "width") == 0)
	{
		width = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "line_size") == 0)
	{
		line_size = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_size") == 0)
	{
		font_size = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_size_prefix") == 0)
	{
		font_size_prefix = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_size_postfix") == 0)
	{
		font_size_postfix = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_offset_top") == 0)
	{
		font_offset_top = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_offset_top_prefix") == 0)
	{
		font_offset_top_prefix = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "font_offset_top_postfix") == 0)
	{
		font_offset_top_postfix = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "align") == 0)
	{
		align = strtoul(value, NULL, 10);
	}
	else if(strcmp(name, "color_prefix") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_prefix[0] = (tmp >> 16) & 0xff;
		color_prefix[1] = (tmp >> 8) & 0xff;
		color_prefix[2] = tmp & 0xff;
	}
	else if(strcmp(name, "color_postfix") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_postfix[0] = (tmp >> 16) & 0xff;
		color_postfix[1] = (tmp >> 8) & 0xff;
		color_postfix[2] = tmp & 0xff;
	}
	else if(strcmp(name, "color_active") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_active[0] = (tmp >> 16) & 0xff;
		color_active[1] = (tmp >> 8) & 0xff;
		color_active[2] = tmp & 0xff;
	}
	else if(strcmp(name, "color_text") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_text[0] = (tmp >> 16) & 0xff;
		color_text[1] = (tmp >> 8) & 0xff;
		color_text[2] = tmp & 0xff;
	}
	else if(strcmp(name, "color_line") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_line[0] = (tmp >> 16) & 0xff;
		color_line[1] = (tmp >> 8) & 0xff;
		color_line[2] = tmp & 0xff;
	}
	else if(strcmp(name, "color_background") == 0)
	{
		tmp = strtoul(value, NULL, 16);
		color_background[0] = (tmp >> 16) & 0xff;
		color_background[1] = (tmp >> 8) & 0xff;
		color_background[2] = tmp & 0xff;
	}
	else if(strcmp(name, "prefix") == 0)
	{
		prefix = strdup(value);
	}
	else if(strcmp(name, "postfix") == 0)
	{
		postfix = strdup(value);
	}

	return 1;
}

short plugin_init_tag(struct tag* tag)
{
	int i;

	if(tag == NULL)
	{
		return -1;
	}

	tag->side = side;
	tag->width = width;
	tag->line_size = line_size;
	tag->font_size = font_size;
	tag->font_size_prefix = font_size_prefix;
	tag->font_size_postfix = font_size_postfix;
	tag->font_offset_top = font_offset_top;
	tag->font_offset_top_prefix = font_offset_top_prefix;
	tag->font_offset_top_postfix = font_offset_top_postfix;
	tag->align = align;
	tag->prefix = NULL;
	tag->postfix = NULL;
	tag->text = NULL;

	for(i = 0; i < 3; ++i)
	{
		tag->color_prefix[i] = color_prefix[i];
		tag->color_postfix[i] = color_postfix[i];
		tag->color_active[i] = color_active[i];
		tag->color_text[i] = color_text[i];
		tag->color_line[i] = color_line[i];
		tag->color_background[i] = color_background[i];
	}

	return 0;
}

void plugin_free()
{
	free(prefix);
	free(postfix);
}