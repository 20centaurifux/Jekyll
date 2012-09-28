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

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <string.h>

#include "completion.h"
#include "pathbuilder.h"

/**
 * @addtogroup Core
 * @{
 */

/*! Comletion counter size. */
#define COMPLETION_COUNTER_SIZE sizeof(gint32)

/*! Size of each stored block (string length + counter size). */
static const gint block_size = COMPLETION_STRING_SIZE + COMPLETION_COUNTER_SIZE;

void
completion_populate_entry_completion(const gchar *filename, GtkEntryCompletion *completion)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *path;
	GFile *file;
	GFileInputStream *stream;
	gint column;
	GError *err = NULL;
	gchar buffer[block_size];
	const gchar *ptr;
	gssize size;

	g_assert(filename != NULL);
	g_assert(GTK_IS_ENTRY_COMPLETION(completion));

	model = gtk_entry_completion_get_model(GTK_ENTRY_COMPLETION(completion));
	column = gtk_entry_completion_get_text_column(GTK_ENTRY_COMPLETION(completion));

	path = g_build_filename(pathbuilder_get_user_application_directory(), G_DIR_SEPARATOR_S, filename, NULL);
	file = g_file_new_for_path(path);

	/* load strings from file & append entries to GtkEntryCompletion's model */
	g_debug("Trying to load strings list from: \"%s\"", path);

	if(g_file_query_exists(file, NULL))
	{
		g_debug("Found file, reading content...");

		if((stream = g_file_read(file, NULL, &err)))
		{
			model = gtk_entry_completion_get_model(GTK_ENTRY_COMPLETION(completion));

			do
			{
				if((size = g_input_stream_read(G_INPUT_STREAM(stream), buffer, block_size, NULL, &err)) == block_size)
				{
					ptr = buffer + COMPLETION_COUNTER_SIZE;
					g_debug("Found text: \"%s\"", ptr);

					if(GTK_IS_LIST_STORE(model))
					{
						gtk_list_store_append(GTK_LIST_STORE(model), &iter);
						gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, ptr, -1);
					}
					else
					{
						g_warning("gtk_entry_completion_get_model() is not GTK_LIST_STORE");
					}
				}
				else
				{
					if(size)
					{
						g_warning("Invalid block size: %d", size);
					}

					break;
				}
			} while(!err && size);

			g_debug("Closing file: \"%s\"", path);
			g_input_stream_close(G_INPUT_STREAM(stream), NULL, &err);
			g_object_unref(stream);
		}
		else
		{
			g_warning("Couldn't read file: \"%s\"", path);
		}
	}

	if(err)
	{
		g_warning("%s", err->message);
		g_error_free(err);
	}

	g_object_unref(file);
	g_free(path);
}

static void
_completion_create_initial_file(GFile *file, const gchar *text)
{
	gchar *path;
	gint len;
	GFileOutputStream *stream;
	gchar buffer[block_size];

	g_assert(file != NULL);
	g_assert(text != NULL);

	len = strlen(text);

	g_return_if_fail(len <= COMPLETION_STRING_SIZE);

	path = g_file_get_path(file);

	g_debug("Creating file: \"%s\"", path);

	if((stream = g_file_create(file, G_FILE_CREATE_PRIVATE, NULL, NULL)))
	{
		memset(buffer, 0, block_size);
		memcpy(buffer + COMPLETION_COUNTER_SIZE, text, len);

		g_debug("Appending new text: \"%s\"", text);
		g_output_stream_write(G_OUTPUT_STREAM(stream), buffer, block_size, NULL, NULL);
		g_output_stream_flush(G_OUTPUT_STREAM(stream), NULL, NULL);

		g_output_stream_close(G_OUTPUT_STREAM(stream), NULL, NULL);
		g_object_unref(stream);
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
	GInputStream *in_stream;
	GOutputStream *out_stream;
	gchar buffer[block_size];
	const gchar *ptr;
	GError *err = NULL;
	gboolean found = FALSE;
	gint32 counter = 0;
	gssize size;

	g_assert(file != NULL);
	g_assert(text != NULL);

	path = g_file_get_path(file);

	g_debug("Opening file (rw): \"%s\"", path);

	if((file_stream = g_file_open_readwrite(file, NULL, &err)))
	{
		/* search for given text */
		g_debug("Searching text...");
		in_stream = g_io_stream_get_input_stream(G_IO_STREAM(file_stream));
		out_stream = g_io_stream_get_output_stream(G_IO_STREAM(file_stream));

		do
		{
			if((size = g_input_stream_read(in_stream, buffer, block_size, NULL, &err)) == block_size)
			{
				ptr = buffer + COMPLETION_COUNTER_SIZE;
				g_debug("Found text: \"%s\"", ptr);

				if(!strcmp(ptr, text))
				{
					found = TRUE;

					/* update counter */
					memcpy(&counter, buffer, COMPLETION_COUNTER_SIZE);

					if(counter != G_MAXINT32)
					{
						counter++;
					}

					g_debug("updating counter: %d", counter);

					if(g_seekable_seek(G_SEEKABLE(out_stream), -block_size, G_SEEK_CUR, NULL, &err))
					{
						g_output_stream_write(out_stream, &counter, COMPLETION_COUNTER_SIZE, NULL, &err);
					}
					else
					{
						g_warning("g_seekable_seek() failed");
					}
				}
			}
			else
			{
				if(size)
				{
					g_warning("Invalid block size: %d", size);
				}
			}
		} while(!err && !found && size);

		if(!found)
		{
			/* append text to file */
			g_debug("Appending new text: \"%s\"", text);

			memset(buffer, 0, block_size);
			memcpy(buffer + COMPLETION_COUNTER_SIZE, text, strlen(text));

			g_output_stream_write(out_stream, buffer, block_size, NULL, &err);
		}

		g_debug("Closing file");
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
completion_append_string(const gchar *filename, const gchar *text)
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

