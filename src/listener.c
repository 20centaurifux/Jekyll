/***************************************************************************
    begin........: January 2011
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
 * \file listener.c
 * \brief A listener allow only one instance of the program.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 27. April 2011
 */

#include <gio/gio.h>

#include "listener.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef G_OS_UNIX
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "application.h"
#endif

/**
 * @addtogroup Core 
 * @{
 */

/*! Buffer size of the read buffer. */
#define LISTENER_READ_BUFFER_SIZE            256
/*! Buffer size of the message parser. */
#define LISTENER_MESSAGE_PARSER_BUFFER_SIZE  512

#ifdef G_OS_WIN32
/*! Name of the pipe. */
#define LISTENER_WIN32_PIPE_NAME             "\\\\.\\pipe\\jekyll"
/*! Server timeout. */
#define LISTENER_WIN32_SERVER_TIMEOUT        1000
/*! Client timeout. */
#define LISTENER_WIN32_CLIENT_TIMEOUT        30000
#endif

#ifdef G_OS_UNIX
/*! Filename of the FIFO. */
#define LISTENER_FIFO_FILENAME               ".fifo"
/*! Filename of the lockfile. */
#define LISTENER_LOCK_FILENAME               ".lock"
#endif

/*! The worker thread. */
static GThread *worker = NULL;
/*! Cancellable to abort the thread. */
static GCancellable *cancellable = NULL;
/*! A list containing callback functions. */
static GHashTable *callbacks = NULL;
/*! A mutex to protect the callback list. */
static GMutex *callback_mutex = NULL;

#ifdef G_OS_UNIX
/*! The lockfile. */
gint listener_lock_file_fd = -1;
#endif

/**
 * \struct _ListenerCallbackData
 * \brief Holds a callback function, user data and a destroy function.
 */
typedef struct
{
	/*! An unique identifier. */
	gint id;
	/*! A callback function. */
	ListenerCallback callback;
	/*! Callback data. */
	gpointer user_data;
	/*! A function to free the user data. */
	GFreeFunc free_func;
} _ListenerCallbackData;

/**
 * \struct _ListenerMessageData
 * \brief Holds message data.
 */
typedef struct
{
	/*! Message code. */
	gint32 code;
	/*! Message text. */
	const gchar *text;
} _ListenerMessageData;

/*
 *	helpers:
 */
static void
_listener_unref(void)
{
	if(cancellable)
	{
		g_object_unref(cancellable);
		cancellable = NULL;
	}

	if(callback_mutex)
	{
		g_mutex_free(callback_mutex);
		callback_mutex = NULL;
	}

	if(callbacks)
	{
		g_hash_table_unref(callbacks);
		callbacks = NULL;
	}

	worker = NULL;
}

/*
 *	hashtable functions:
 */
static void
_listener_destroy_hash_value(_ListenerCallbackData *data)
{
	g_debug("Freeing memory allocated for listener callback: %d", data->id);

	if(data->user_data && data->free_func)
	{
		data->free_func(data->user_data);
	}

	g_free(data);
}

static void
_listener_invoke_callback(gint id, _ListenerCallbackData *callback, _ListenerMessageData *arg)
{
	g_debug("Invoking listener callback: %d", id);
	callback->callback(arg->code, arg->text, callback->user_data);
}

static void
_listener_handle_message(guchar *message, gssize length)
{
	static gchar buffer[LISTENER_MESSAGE_PARSER_BUFFER_SIZE] = { 0 };
	static gint offset = 0;
	gint i;
	gint code = 0;
	_ListenerMessageData arg;

	/* parse message text */
	for(i = 0; i < length; ++i)
	{
		if(offset < (LISTENER_MESSAGE_PARSER_BUFFER_SIZE - 1))
		{
			buffer[offset] = message[i];
		}
		else
		{
			g_warning("%s: message text exceeds buffer size", __func__);
		}

		/* check if message code has been read (first 4 bytes) & if current byte is zero */
		if(offset >= 4 && !message[i])
		{
			buffer[offset] = '\0';

			/* reset offset & deserialize message code */
			offset = 0;

			code = 0;
			code |= buffer[3] & 0xFF;
			code <<= 8;
			code |= buffer[2] & 0xFF;
			code <<= 8;
			code |= buffer[1] & 0xFF;
			code <<= 8;
			code |= buffer[0] & 0xFF;

			/* invoke callback functions */
			g_debug("Processing message: code: %d, text: \"%s\"", code, buffer + 4);
			arg.code = code;
			arg.text = buffer + 4;

			g_mutex_lock(callback_mutex);
			g_hash_table_foreach(callbacks, (GHFunc)_listener_invoke_callback, &arg);
			g_mutex_unlock(callback_mutex);
		}
		else
		{
			if(offset < (LISTENER_MESSAGE_PARSER_BUFFER_SIZE - 1))
			{
				++offset;
			}
		}
	}
}

/*
 *	windows functions:
 */
#ifdef G_OS_WIN32
static gchar *
_listener_win32_build_pipename(void)
{
	gchar *pipename;
	gchar *identifier;

	identifier = g_compute_checksum_for_string(G_CHECKSUM_SHA256, g_get_user_name(), -1);
	pipename = g_strdup_printf("%s~%s", LISTENER_WIN32_PIPE_NAME, identifier);
	g_free(identifier);

	return pipename;
}

static HANDLE
_listener_win32_create_server_pipe(DWORD *err)
{
	gchar *pipename;
	HANDLE pipe;

	pipename = _listener_win32_build_pipename();
	g_debug("Server pipe name: \"%s\"", pipename);
	pipe = CreateNamedPipe(pipename,
	                       PIPE_ACCESS_INBOUND,
	                       PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
	                       1,
	                       LISTENER_READ_BUFFER_SIZE, /* TOCHECK */
	                       0,
	                       LISTENER_WIN32_CLIENT_TIMEOUT,
	                       NULL);
	g_free(pipename);

	if(pipe == INVALID_HANDLE_VALUE)
	{
		*err = GetLastError();
		pipe = NULL;
		g_debug("Couldn't create server pipe");
	}

	return pipe;
}

static void
_listener_win32_handle_client_request(HANDLE pipe)
{
	guchar message[LISTENER_READ_BUFFER_SIZE + 1];
	DWORD count;

	g_debug("Client connected");

	while(ReadFile(pipe, message, sizeof(message), &count, NULL)) // TOCHECK
	{
		_listener_handle_message(message, (gssize)count);
	}

	DisconnectNamedPipe(pipe);
}

static gpointer
_listener_win32_server_worker(HANDLE pipe)
{
	DWORD pipe_err = 0;

	/* wait for client(s) */
	while(!g_cancellable_is_cancelled(cancellable))
	{
		if(pipe_err == 0 && ConnectNamedPipe(pipe, NULL))
		{
			/* handle client request */
			_listener_win32_handle_client_request(pipe);
		}
	}

	/* close server pipe */
	CloseHandle(pipe);

	return NULL;
}

static HANDLE
_listener_win32_create_client_pipe(void)
{
	gchar *pipename;
	HANDLE pipe = NULL;
	gint round = 0;

	pipename = _listener_win32_build_pipename();
	g_debug("Client pipe name: \"%s\"", pipename);

	while(!pipe && round < 10)
	{
		pipe = CreateFile(pipename,
				  GENERIC_WRITE,
				  FILE_SHARE_WRITE,
				  NULL,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
				  NULL);

		if(pipe == INVALID_HANDLE_VALUE)
		{
			pipe = NULL;

			if(GetLastError() == ERROR_PIPE_BUSY)
			{
				++round;
				g_usleep(50000);
			}
			else
			{
				break;
			}
		}
	}

	g_free(pipename);

	return pipe;
}

static gssize
_listener_win32_write(const guchar *buffer, gssize length)
{
	HANDLE pipe;
	DWORD written;
	gssize result = -1;

	/* create client pipe & send message */
	g_debug("Creating client pipe");
	if((pipe = _listener_win32_create_client_pipe()))
	{
		g_debug("Waiting for client pipe");
		if((GetLastError() != ERROR_PIPE_BUSY) || !WaitNamedPipe(LISTENER_WIN32_PIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT))
		{
			g_debug("Sending message to server");
			if(WriteFile(pipe, (LPSTR)buffer, length, &written, NULL))
			{
				result = (gssize)written;
			}
			else
			{
				g_warning("Client couldn't write to pipe");
			}
		}

		/* close pipe */
		CloseHandle(pipe);
	}
	else
	{
		g_debug("_listener_create_win32_client_pipe() failed");
	}

	return result;
}

static ListenerInitResult
_listener_win32_init(void)
{
	HANDLE pipe;
	DWORD pipe_err = 0;
	GError *err = NULL;
	ListenerInitResult result = LISTENER_RESULT_ABORT;

	/* create server pipe */
	g_debug("Creating initial server pipe");
	if((pipe = _listener_win32_create_server_pipe(&pipe_err)))
	{
		/* create hashtable */
		g_debug("Creating callback table");
		callbacks = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GFreeFunc)_listener_destroy_hash_value);
		callback_mutex = g_mutex_new();

		/* start worker */
		g_debug("Starting server worker...");
		cancellable = g_cancellable_new();

		if((worker = g_thread_create((GThreadFunc)_listener_win32_server_worker, pipe, TRUE, NULL)))
		{
			result = LISTENER_RESULT_OK;
		}
		else
		{
			if(err)
			{
				g_warning("%s", err->message);
				g_error_free(err);
			}
		}
	}
	else
	{
		if(pipe_err == ERROR_PIPE_BUSY)
		{
			result = LISTENER_RESULT_FOUND_INSTANCE;
		}
	}

	return result;
}
#endif

/*
 *	UNIX functions:
 */
#ifdef G_OS_UNIX
static gchar *
_listener_fifo_build_filename(void)
{
	return g_build_filename(g_get_home_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, LISTENER_FIFO_FILENAME, NULL);
}

static gchar *
_listener_lock_build_filename(void)
{
	return g_build_filename(g_get_home_dir(), G_DIR_SEPARATOR_S, CONFIG_DIR, LISTENER_LOCK_FILENAME, NULL);
}

static gint
_listener_lock_create(void)
{
	gchar *filename;
	gint fd;
	struct flock fl = { F_WRLCK, SEEK_SET, 0, 0, getpid() };
	gboolean success = FALSE;

	/* build filaname */
	filename = _listener_lock_build_filename();

	/* set filepermission */
	chmod(filename, 700);

	/* open lockfile */
	g_debug("Opening lockfile (O_WRONLY | O_CREAT): \"%s\"", filename);
	if((fd = open(filename, O_WRONLY | O_CREAT)) != -1)
	{
		/* create lock */
		g_debug("Locking file");
		if(fcntl(fd, F_SETLK, &fl) != -1)
		{
			g_debug("Lock created successfully");
			success = TRUE;
		}
		else
		{
			g_debug("Couldn't lock file, errno=%d", errno);
		}
	}
	else
	{
		g_debug("Couldn't open lockfile, errno=%d", errno);
	}

	/* cleanup */
	g_free(filename);

	if(!success)
	{
		close(fd);
		fd = -1;
	}

	return fd;
}

static void
_listener_lock_remove(gint fd)
{
	struct flock fl = { F_WRLCK, SEEK_SET, 0, 0, getpid() };

	if(fcntl(fd, F_SETLK, &fl) == -1)
	{
		g_warning("Couldn't unlock file, errno=%d", errno);
	}
}

static gint
_listener_fifo_server_open(void)
{
	gchar *filename;
	gint fd;

	/* build filename */
	filename = _listener_fifo_build_filename();
	g_debug("FIFO filename: \"%s\"", filename);

	/* create FIFO */
	if(!(fd = mkfifo(filename, 0666)))
	{
		g_debug("FIFO created successfully");
	}

	/* open FIFO */
	g_debug("Opening FIFO (O_RDONLY)");
	fd = open(filename, O_RDONLY | O_NONBLOCK);

	/* cleanup */
	g_free(filename);

	return fd;
}

static gpointer
_listener_fifo_server_worker(gint fd)
{
	gssize bytes;
	guchar buffer[LISTENER_READ_BUFFER_SIZE];
	gint flags;

	/* set blocking */
	flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

	/* wait for client(s) */
	while(!g_cancellable_is_cancelled(cancellable))
	{
		if((bytes = read(fd, buffer, LISTENER_READ_BUFFER_SIZE)) > 0)
		{
			_listener_handle_message(buffer, bytes);
		}
		else
		{
			g_usleep(1000000);
		}
	}

	/* close file */
	close(fd);

	return NULL;
}

static gssize
_listener_fifo_write(const guchar *buffer, gssize length)
{
	gchar *filename;
	gint fd;
	gssize result = -1;

	/* build filename */
	filename = _listener_fifo_build_filename();
	g_debug("FIFO filename: \"%s\"", filename);

	/* open FIFO */
	g_debug("Opening FIFO (O_WRONLY)");
	if((fd = open(filename, O_WRONLY | O_NONBLOCK)))
	{
		/* write data & close file */
		result = write(fd, buffer, length);
		close(fd);
	}
	else
	{
		g_warning("Couldn't write to FIFO");
	}

	/* cleanup */
	g_free(filename);

	return result;
}

static ListenerInitResult
_listener_fifo_init(void)
{
	gint fd;
	GError *err = NULL;
	ListenerInitResult result = LISTENER_RESULT_ABORT;

	/* create lockfile */
	if((listener_lock_file_fd = _listener_lock_create()) == -1)
	{
		if(errno == EACCES || errno == EAGAIN)
		{
			return LISTENER_RESULT_FOUND_INSTANCE;
		}
		else
		{
			return LISTENER_RESULT_ABORT;
		}
	}

	/* open FIFO for reading */
	g_debug("Opening FIFO for reading");
	if((fd = _listener_fifo_server_open()) > 0)
	{
		/* create hashtable */
		g_debug("Creating callback table");
		callbacks = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GFreeFunc)_listener_destroy_hash_value);
		callback_mutex = g_mutex_new();

		/* start worker */
		g_debug("Starting server worker...");
		cancellable = g_cancellable_new();

		if((worker = g_thread_create((GThreadFunc)_listener_fifo_server_worker, GINT_TO_POINTER(fd), TRUE, NULL)))
		{

			result = LISTENER_RESULT_OK;
		}
		else if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
	}

	return result;
}
#endif

/*
 *	public:
 */
ListenerInitResult
listener_init(void)
{
	ListenerInitResult result = LISTENER_RESULT_ABORT;

	g_assert(worker == NULL);
	g_assert(cancellable == NULL);
	g_assert(callbacks == NULL);
	g_assert(callback_mutex == NULL);

	#ifdef G_OS_WIN32
	result = _listener_win32_init();
	#endif

	#ifdef G_OS_UNIX
	g_assert(listener_lock_file_fd == -1);

	result = _listener_fifo_init();
	#endif

	if(result != LISTENER_RESULT_OK)
	{
		_listener_unref();
	}

	return result;
}

gssize
listener_send_message(gint32 code, const gchar *text)
{
	guchar *buffer;
	gint size;
	gssize result = -1;

	/* create buffer */
	size = text ? strlen(text) + 5 : 5;
	buffer = (guchar *)g_malloc0(size);

	/* serialize message code */
	buffer[0] = code & 0xFF;
	buffer[1] = code >> 8 & 0xFF;
	buffer[2] = code >> 16 & 0xFF;
	buffer[3] = code >> 24 & 0xFF;

	/* copy message text (if given) */
	if(text)
	{
		memcpy(buffer + 4, text, size - 4);
	}

	/* write data to pipe */
	#ifdef G_OS_WIN32
	result = _listener_win32_write(buffer, size);
	#endif

	#ifdef G_OS_UNIX
	result = _listener_fifo_write(buffer, size);
	#endif

	/* cleanup */
	g_free(buffer);

	return result;
}

gint
listener_add_callback(ListenerCallback callback, gpointer user_data, GFreeFunc free_func)
{
	static gint id = 0;
	_ListenerCallbackData *data;

	g_assert(callbacks != NULL);
	g_assert(callback_mutex != NULL);

	g_debug("Adding listener callback: %d", ++id);

	/* insert callback into hashtable */
	g_mutex_lock(callback_mutex);
	data = (_ListenerCallbackData *)g_malloc(sizeof(_ListenerCallbackData));
	data->id = id;
	data->callback = callback;
	data->user_data = user_data;
	data->free_func = free_func;
	g_hash_table_insert(callbacks, GINT_TO_POINTER(data->id), data);
	g_mutex_unlock(callback_mutex);

	return data->id;
}

void
listener_remove_callback(gint id)
{
	g_assert(callbacks != NULL);
	g_assert(callback_mutex != NULL);

	g_debug("Removing listener callback: %d", id);

	g_mutex_lock(callback_mutex);
	g_hash_table_remove(callbacks, GINT_TO_POINTER(id));
	g_mutex_unlock(callback_mutex);
}

void listener_shutdown(void)
{
	g_assert(worker != NULL);
	g_assert(cancellable != NULL);
	g_assert(callback_mutex != NULL);

	g_debug("Cancelling listener");
	listener_send_message(LISTENER_MESSAGE_PING, "Application shutdown");
	g_cancellable_cancel(cancellable);

	/* join thread */
	g_debug("Joining listener thread...");
	g_thread_join(worker);

	/* remove lock */
	#ifdef G_OS_UNIX
	g_assert(listener_lock_file_fd != -1);
	_listener_lock_remove(listener_lock_file_fd);
	#endif

	/* destroy objects */
	g_debug("Destroying listener references");
	_listener_unref();
}

/**
 * @}
 */

