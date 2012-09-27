/***************************************************************************
    begin........: September 2012
    copyright....: Sebastian Fedrau
    email........: lord-kefir@arcor.de
 ***************************************************************************/

/***************************************************************************
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 ***************************************************************************/
/*!
 * \file completion.c
 * \brief Organize strings for autocompletion support.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 26. September 2012
 */

#include <gio/gio.h>
#include <string.h>

#include "completion.h"
#include "pathbuilder.h"

/**
 * @addtogroup Core
 * @{
 */

GList *
completion_load_file(const char *filename)
{
	char *path;
	GFile *file;
	GFileInputStream *file_stream;
	GDataInputStream *stream;
	gchar *line;
	GList *strings = NULL;

	path = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, filename, NULL);
	file = g_file_new_for_path(path);

	g_debug("Trying to load string list from: \"%s\"", path);

	if(g_file_query_exists(file, NULL))
	{
		g_debug("Found file, reading content...");

		if((file_stream = g_file_read(file, NULL, NULL)))
		{
			stream = g_data_input_stream_new(G_INPUT_STREAM(file_stream));

			while((line = g_data_input_stream_read_line(stream, NULL, NULL, NULL)))
			{
				strings = g_list_append(strings, g_strdup(line + sizeof(gint32)));
				g_free(line);
			}

			g_debug("Closing file");
			g_input_stream_close(G_INPUT_STREAM(stream), NULL, NULL);
			g_object_unref(stream);
			g_object_unref(file_stream);
		}
	}

	g_object_unref(file);
	g_free(path);

	return strings;
}

static void
_completion_create_initial_file(GFile *file, const char * restrict path, const char * restrict text)
{
	GFileOutputStream *file_stream;
	GOutputStream *stream;
	gchar *buffer;
	gint len;
	gint size;
	gint32 zero = 0;

	g_debug("Creating file: \"%s\"", path);

	if((file_stream = g_file_create(file, G_FILE_CREATE_PRIVATE, NULL, NULL)))
	{
		len = strlen(text);
		size = len + sizeof(gint32) + 1;
		buffer = g_malloc(size);

		memcpy(buffer, &zero, sizeof(gint32));
		memcpy(buffer + sizeof(gint32), text, len);
		buffer[size - 1] = '\n';

		stream = G_OUTPUT_STREAM(file_stream);
		g_output_stream_write(stream, buffer, size, NULL, NULL);

		g_output_stream_flush(stream, NULL, NULL);
		g_output_stream_close(stream, NULL, NULL);
		g_object_unref(file_stream);

		g_free(buffer);
	}
	else
	{
		g_warning("Couldn't create file: \"%s\"", path);
	}
}

void
completion_append_string(const char *filename, const char *text)
{
	char *path;
	GFile *file;

	g_assert(filename != NULL);
	g_assert(text != NULL);

	path = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, filename, NULL);
	file = g_file_new_for_path(path);

	g_debug("Testing if file does exist: \"%s\"", path);

	if(g_file_query_exists(file, NULL))
	{
		/* TODO */
	}
	else
	{
		_completion_create_initial_file(file, path, text);
	}

	g_object_unref(file);
	g_free(path);
}

/**
 * @}
 */

