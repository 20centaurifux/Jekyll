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
 * \date 27. September 2012
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

	g_assert(filename != NULL);

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
_completion_create_initial_file(GFile *file, const char *text)
{
	gchar *path;
	GFileOutputStream *file_stream;
	GOutputStream *stream;
	gchar *buffer;
	gint len;
	gint size;
	gint32 zero = 0;

	g_assert(file != NULL);
	g_assert(text != NULL);

	path = g_file_get_path(file);

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

	g_free(path);
}

static void
_completion_update_text(GFile *file, const gchar *text)
{
	gchar *path;
	GFileIOStream *file_stream;
	GDataInputStream *in_stream;
	GOutputStream *out_stream;
	GError *err = NULL;
	gchar *line;
	const char *ptr;
	gboolean found = FALSE;
	gint32 counter = 0;

	g_assert(file != NULL);
	g_assert(text != NULL);

	path = g_file_get_path(file);

	g_debug("Opening file (rw): \"%s\"", path);

	if((file_stream = g_file_open_readwrite(file, NULL, &err)))
	{
		// search for given text:
		g_debug("Searching text...");
		in_stream = g_data_input_stream_new(g_io_stream_get_input_stream(G_IO_STREAM(file_stream)));
		out_stream = g_io_stream_get_output_stream(G_IO_STREAM(file_stream));

		while((line = g_data_input_stream_read_line(in_stream, NULL, NULL, &err)) && !err && !found)
		{
			ptr = line + sizeof(gint32);

			if(!g_strcmp0(ptr, text))
			{
				// found text => update counter:
				g_debug("Found text, seeking");
				found = TRUE;

				if(g_seekable_seek(G_SEEKABLE(file_stream), -(strlen(text) + 5), G_SEEK_CUR, NULL, &err))
				{
					memcpy(&counter, line, sizeof(gint32));

					if(counter != G_MAXINT)
					{
						g_debug("new counter: %d", ++counter);
					}

					g_output_stream_write(out_stream, &counter, sizeof(gint32), NULL, &err);
				}
				else
				{
					g_warning("seek failed in file: \"%s\"", path);
				}
			}

			g_free(line);
		}

		if(!found)
		{
			// append text to file:
			g_debug("Appending new text: \"%s\"", text);
			g_output_stream_write(out_stream, &counter, sizeof(gint32), NULL, NULL);
			g_output_stream_write(out_stream, text, strlen(text), NULL, NULL);
			g_output_stream_write(out_stream, "\n", 1, NULL, NULL);
		}

		g_debug("Closing file");
		g_object_unref(in_stream);
		g_io_stream_close(G_IO_STREAM(file_stream), NULL, NULL);
		g_object_unref(file_stream);
	}
	else
	{
		g_warning("Couldn't open file (rw): \"%s\"", path);
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_free(path);
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
		_completion_update_text(file, text);
	}
	else
	{
		_completion_create_initial_file(file, text);
	}

	g_object_unref(file);
	g_free(path);
}

/**
 * @}
 */

