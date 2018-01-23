#include "excalibar.h"
#include <string.h>

short tag_free(struct tag* tag)
{
	if(tag == NULL)
	{
		return -1;
	}

	if(tag->prefix != NULL)
	{
		free(tag->prefix);
	}

	if(tag->postfix != NULL)
	{
		free(tag->postfix);
	}

	if(tag->text != NULL)
	{
		free(tag->text);
	}

	return 0;
}

// sets the tag's strings, allowing the user to reuse malloc'd strings
short tag_set_strings(struct tag* tag, short dup, char* prefix,
	char* text, char* postfix)
{
	if(tag == NULL)
	{
		return -1;
	}

	// dups pointed strings and updates the local adresses
	if(dup > 0)
	{
		if(prefix != NULL)
		{
			prefix = strdup(prefix);
		}

		if(postfix != NULL)
		{
			postfix = strdup(postfix);
		}

		if(text != NULL)
		{
			text = strdup(text);
		}
	}

	// frees existing strings and updates the adresses
	if(prefix != NULL)
	{
		if(tag->prefix != NULL)
		{
			free(tag->prefix);
		}

		tag->prefix = prefix;
	}

	if(postfix != NULL)
	{
		if(tag->postfix != NULL)
		{
			free(tag->postfix);
		}

		tag->postfix = postfix;
	}

	if(text != NULL)
	{
		if(tag->text != NULL)
		{
			free(tag->text);
		}

		tag->text = text;
	}

	return 0;
}
