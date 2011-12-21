/***************************************************************************
    begin........: June 2010
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
 * \file urlopener.c
 * \brief Opening urls.
 * \author Sebastian Fedrau <lord-kefir@arcor.de>
 * \version 0.1.0
 * \date 18. February 2011
 */

#include <glib.h>
#include <string.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#include "urlopener.h"

/**
 * @addtogroup Core
 * @{
 */

/*
 *	helpers for non-windows operating systems:
 */
#ifndef G_OS_WIN32
/*! A list containing default browser launcher. */
const gchar *default_browser_launcher[] =
{
	"xdg-open",
	"kfmclient",
	"gnome-open",
	NULL,
};

/*! A list containing required arguments for the defined default launcher. */
const char *default_browser_arguments[] =
{
	NULL,
	"exec",
	NULL
};

static gboolean
_url_opener_find_default_launcher_in_directory(const gchar *path, gchar **executable, gchar **arguments)
{
	GDir *dir;
	const gchar *entry;
	gchar *filename;
	const gchar **launcher;
	const gchar **arg;
	GError *err = NULL;

	g_assert(executable != NULL && *executable == NULL);
	g_assert(arguments != NULL && *executable == NULL);

	/* open directory and iterate files */
	if((dir = g_dir_open(path, 0, &err)))
	{
		while((entry = g_dir_read_name(dir)))
		{
			launcher = default_browser_launcher;
			arg = default_browser_arguments;
		
			while(*launcher)
			{
				if(!g_strcmp0(*launcher, entry))
				{
					/* check if found file is an executable */
					filename = g_build_filename(path, G_DIR_SEPARATOR_S, entry, NULL);
					if(g_file_test(filename, G_FILE_TEST_IS_EXECUTABLE))
					{
						*executable = filename;
						if(arg)
						{
							*arguments = g_strdup(*arg);
						}

						return TRUE;
					}

					g_free(filename);
				}

				++launcher;
				++arg;
			}
		}

		/* close directory  */
		g_dir_close(dir);
	}
	else
	{
		if(err)
		{
			g_warning("%s", err->message);
			g_error_free(err);
		}
		else
		{
			g_warning("Couldn't open directory: \"%s\"", path);
		}
	}

	return FALSE;
}

static gboolean
_url_opener_find_default_launcher(gchar **executable, gchar **arguments)
{
	const gchar *path_env;
	gchar **paths;
	guint i;
	guint length;

	g_assert(executable != NULL && *executable == NULL);
	g_assert(arguments != NULL && *executable == NULL);

	/* get default application directories from environment & search for launcher */
	if((path_env = g_getenv("PATH")))
	{
		#ifdef G_OS_WIN32
		if((paths = g_strsplit(path_env, ";", 0)))
		#else
		if((paths = g_strsplit(path_env, ":", 0)))
		#endif
		{
			length = g_strv_length(paths);
			for(i = 0; i < length; ++i)
			{
				g_debug("Searching for default browser launcher in \"%s\"", paths[i]);
				if(_url_opener_find_default_launcher_in_directory(paths[i], executable, arguments))
				{
					return TRUE;
				}
			}

			g_strfreev(paths);
		}
	}

	return FALSE;
}
#endif

/*
 *	UrlOpenerExecutable
 */

/**
 * \struct _UrlOpenerExecutable
 *
 * \brief Stores the filename and required arguments of an executable used to open an url.
 */
typedef struct
{
	/*! Padding. */
	UrlOpener padding;
	/*! Filename of the executable used to open an url. */
	gchar *executable;
	/*! Additional arguments. */
	gchar *arguments;
} _UrlOpenerExecutable;

static gboolean
_url_opener_executable_launche(UrlOpener *opener, const gchar *url)
{
	_UrlOpenerExecutable *opener_exe = (_UrlOpenerExecutable *)opener;
	gchar *cmd;
	GError *err = NULL;
	gboolean ret = TRUE;

	/* build command */
	g_debug("Opening \"%s\" using \"%s\"", url, opener_exe->executable);
	if(opener_exe->arguments)
	{
		cmd = g_strdup_printf("\"%s\" %s \"%s\"", opener_exe->executable, opener_exe->arguments, url);
	}
	else
	{
		cmd = g_strdup_printf("\"%s\" \"%s\"", opener_exe->executable, url);
	}
	g_debug("Executing \"%s\"", cmd);

	/* execute command */
	g_spawn_command_line_async(cmd, &err);

	if(err)
	{
		ret = FALSE;
		g_warning("%s", err->message);
		g_error_free(err);
	}

	/* free memory  */
	g_free(cmd);

	return ret;
}

static void
_url_opener_executable_free(UrlOpener *opener)
{
	_UrlOpenerExecutable *opener_exe = (_UrlOpenerExecutable *)opener;

	g_free(opener_exe->executable);
	g_free(opener_exe->arguments);
	g_slice_free1(sizeof(_UrlOpenerExecutable), opener_exe);
}

UrlOpener *
url_opener_new_executable(const gchar *executable, const gchar *arguments)
{
	static UrlOpenerFuncs funcs = { _url_opener_executable_launche, _url_opener_executable_free };
	_UrlOpenerExecutable *opener_exe;

	if(!g_file_test(executable, G_FILE_TEST_IS_EXECUTABLE))
	{
		g_warning("Couldn't create _UrlOpenerExecutable: \"%s\" is not executable", executable);
		return NULL;
	}

	opener_exe = (_UrlOpenerExecutable *)g_slice_alloc(sizeof(_UrlOpenerExecutable));
	memset(opener_exe, 0, sizeof(_UrlOpenerExecutable));

	opener_exe->executable = g_strdup(executable);
	if(arguments)
	{
		opener_exe->arguments = g_strdup(arguments);
	}

	((UrlOpener *)opener_exe)->funcs = &funcs;

	return (UrlOpener *)opener_exe;
}

/*
 *	UrlOpenerWin32
 */
#ifdef G_OS_WIN32
typedef HINSTANCE (*_ShellExecuteFunc)(HWND hwnd, LPCTSTR lpOperation, LPCTSTR lpFile, LPCTSTR lpParameters, LPCTSTR lpDirectory, INT nShowCmd);

typedef struct
{
	UrlOpener padding;
	HMODULE hmodule;
	_ShellExecuteFunc shell_execute;
} _UrlOpenerWin32;

static gboolean
_url_opener_win32_open(UrlOpener *opener, const gchar *url)
{
	_UrlOpenerWin32 *opener_win32 = (_UrlOpenerWin32 *)opener;
	return opener_win32->shell_execute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL | SW_MAXIMIZE) ? TRUE : FALSE;
}

static void
_url_opener_win32_free(UrlOpener *opener)
{
	_UrlOpenerWin32 *opener_win32 = (_UrlOpenerWin32 *)opener;

	g_debug("Freeing library \"shell32.dll\"");
	FreeLibrary(opener_win32->hmodule);

	g_debug("Freeing _UrlOpenerWin32 structure");
	g_slice_free1(sizeof(_UrlOpenerWin32), opener_win32);
}

UrlOpener *
url_opener_new_win32(void)
{
	static UrlOpenerFuncs funcs = { _url_opener_win32_open, _url_opener_win32_free };
	_UrlOpenerWin32 *opener_win32;

	opener_win32 = (_UrlOpenerWin32 *)g_slice_alloc(sizeof(_UrlOpenerWin32));
	memset(opener_win32, 0, sizeof(_UrlOpenerWin32));

	g_debug("Loading library \"shell32.dll\"");
	if((opener_win32->hmodule = LoadLibrary("shell32.dll")))
	{
			if((opener_win32->shell_execute = (_ShellExecuteFunc)GetProcAddress(opener_win32->hmodule, "ShellExecuteA")))
			{
				g_debug("Loading function \"ShellExecuteA\"");
				((UrlOpener *)opener_win32)->funcs = &funcs;
			}
			else
			{
				FreeLibrary(opener_win32->hmodule);
				g_warning("Couldn't load function: \"ShellExecuteA\"");
			}
	}
	else
	{
		g_warning("Couldn't load library: \"shell32.dll\"");
	}

	return (UrlOpener *)opener_win32;
}
#endif

/*
 *	public:
 */
gboolean
url_opener_launch(UrlOpener *opener, const gchar *url)
{
	g_return_val_if_fail(opener->funcs != NULL && opener->funcs->open != NULL, FALSE);

	return opener->funcs->open(opener, url);
}

void
url_opener_free(UrlOpener *opener)
{
	g_return_if_fail(opener->funcs != NULL);

	if(opener->funcs != NULL && opener->funcs->free != NULL)
	{
		opener->funcs->free(opener);
	}
}

UrlOpener *
url_opener_new_default(void)
{
	UrlOpener *opener = NULL;

	#ifdef G_OS_WIN32
	opener = url_opener_new_win32();
	#else
	gchar *filename = NULL;
	gchar *arguments = NULL;

	if(_url_opener_find_default_launcher(&filename, &arguments))
	{
		g_debug("Found executable: \"%s\"", filename);
		opener = url_opener_new_executable(filename, arguments);
		g_free(filename);

		if(arguments)
		{
			g_free(arguments);
		}
	}
	#endif

	return opener;
}

/**
 * @}
 */

