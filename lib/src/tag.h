#ifndef EXCALIBAR_TAG_H
#define EXCALIBAR_TAG_H

#include <stdint.h>
#include "excalibar.h"

struct tag
{
	// sizes & style
	int width; // in pixels or chars (codepoints)
	int line_size; // always in pixels
	int font_size;
	int font_size_prefix;
	int font_size_postfix;
	int font_offset_top;
	int font_offset_top_prefix;
	int font_offset_top_postfix;
	short align; // 0=left, 1=right, 2=center
	short side; // 0=left, 1 = right
	// colors
	uint8_t color_prefix[3];
	uint8_t color_postfix[3];
	uint8_t color_text[3];
	uint8_t color_line[3];
	uint8_t color_active[3];
	uint8_t color_background[3];
	// texts
	char* prefix;
	char* postfix;
	char* text;
};

struct config;

short tag_free(struct tag* tag);
short tag_set_strings(struct tag* tag, short dup, char* prefix,
	char* text, char* postfix);

#endif /*EXCALIBAR_TAG_H*/
