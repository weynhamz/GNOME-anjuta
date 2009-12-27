/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
    Copyright (C) 2009 Maxim Ermilov   <zaspire@rambler.ru>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <errno.h>
#include <glib/gi18n.h>

#include "debugger-server.h"

typedef struct _DebuggerServerPrivate DebuggerServerPrivate;
struct _DebuggerServerPrivate
{
	GList* in;
	GList* out;
	int server_socket;
	int socket;
	gboolean work;
	guint id;
};

#define DEBUGGER_SERVER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), DEBUGGER_TYPE_SERVER, DebuggerServerPrivate))

enum
{
	DATA_ARRIVED,
	ERROR_SIGNAL,
	LAST_SIGNAL
};

#define CANT_SEND _("App exited unexpectedly.")
#define CANT_RECV _("App exited unexpectedly.")


static guint server_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (DebuggerServer, debugger_server, G_TYPE_OBJECT);

static void
debugger_server_init (DebuggerServer *object)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	priv->in = NULL;
	priv->out = NULL;
	priv->id = 0;
	priv->server_socket = 0;
	priv->socket = 0;
	priv->work = TRUE;
}

static void
debugger_server_finalize (GObject *object)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);

	g_assert (priv);
	if (priv->socket)
	{
		close (priv->socket);
	}
	if (priv->id)
		g_source_remove (priv->id);

	g_list_foreach (priv->in, (GFunc)g_free, NULL);
	g_list_free (priv->in);
	g_list_foreach (priv->out, (GFunc)g_free, NULL);
	g_list_free (priv->out);

	G_OBJECT_CLASS (debugger_server_parent_class)->finalize (object);
}

static void
debugger_server_data_arrived (DebuggerServer *self)
{
}

static void
debugger_server_error_signal (DebuggerServer *self, const gchar *text)
{
}

static void
debugger_server_class_init (DebuggerServerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DebuggerServerPrivate));

	object_class->finalize = debugger_server_finalize;

	klass->data_arrived = debugger_server_data_arrived;
	klass->error = debugger_server_error_signal;

	server_signals[DATA_ARRIVED] =
		g_signal_new ("data-arrived",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (DebuggerServerClass, data_arrived),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
	server_signals[ERROR_SIGNAL] =
		g_signal_new ("error",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST,
		              G_STRUCT_OFFSET (DebuggerServerClass, error),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__POINTER,
		              G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static gboolean
SourceFunc (gpointer d)
{
	DebuggerServer *object = DEBUGGER_SERVER (d);
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	gint size;

	if (priv->socket == 0)
	{
		fd_set accept_fd;
		struct timeval timeo;
		struct sockaddr *cp = NULL;
		socklen_t sz;

		FD_ZERO(&accept_fd);
		FD_SET(priv->server_socket, &accept_fd);
		timeo.tv_sec=0;
		timeo.tv_usec=1;
		if (select(priv->server_socket + 1,&accept_fd,NULL,NULL,&timeo) > 0)
		{
			if(FD_ISSET(priv->server_socket,&accept_fd))
			{
				if ((priv->socket = accept(priv->server_socket, cp, &sz)) == -1)
				{
					g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, "Can not accept.");
					return FALSE;
				}
				close (priv->server_socket);
			}
		}
	}
	else
	{
		int len;
		if (ioctl (priv->socket, FIONREAD, &len) == -1)
		{
			g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, "Error in ioctl call.");
			return FALSE;
		}
		if (len > 4)
		{
			int in;
			gchar *buf;
			if (recv (priv->socket, &len, 4, 0) == -1)
			{
				g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, CANT_RECV);
				return FALSE;
			}
			if (len <= 0)
			{
				g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, "Incorrect data recived.");
				return FALSE;
			}
			buf = g_new (char, len + 1);
			do
			{
				if (ioctl (priv->socket, FIONREAD, &in) == -1)
				{
					g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, "Error in ioctl call.");
					return FALSE;
				}
				if (in < len)
					usleep (20);
			} while (in < len);
			if (recv (priv->socket, buf, len, 0) == -1)
			{
				g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, CANT_RECV);
				return FALSE;
			}
			buf [len] = '\0';

			priv->in = g_list_append (priv->in, buf);
			g_signal_emit (object, server_signals[DATA_ARRIVED], 0);
		}
		while (priv->out != NULL)
		{
			size = strlen ((gchar*)priv->out->data) + 1;
			if (send(priv->socket, &size, 4, 0) == -1)
			{
				g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, CANT_SEND);
				return FALSE;
			}
			if (send(priv->socket, priv->out->data, size, 0) == -1)
			{
				g_signal_emit (object, server_signals[ERROR_SIGNAL], 0, CANT_SEND);
				return FALSE;
			}
			g_free (priv->out->data);
			priv->out = g_list_delete_link (priv->out, priv->out);
		}
g_signal_emit (object, server_signals[DATA_ARRIVED], 0);//TODO:FIX
	}

	if (!priv->work)
	{
		close (priv->socket);
		priv->socket = 0;
	}
	return priv->work;
}


DebuggerServer*
debugger_server_new (gint port)
{
	DebuggerServer* object = g_object_new (DEBUGGER_TYPE_SERVER, NULL);
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	struct sockaddr_in serverAddr;
	int flag = 1;

	priv->server_socket = socket (AF_INET, SOCK_STREAM, 0);

	if (priv->server_socket == -1)
		return NULL;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr= htonl (INADDR_ANY);
	serverAddr.sin_port = htons (port);
	setsockopt (priv->server_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	if (bind (priv->server_socket, ((struct sockaddr*)&serverAddr), sizeof (serverAddr)) == -1)
	{
		g_warning("%s\n", strerror(errno));
		g_object_unref (object);
		return NULL;
	}
	listen (priv->server_socket , 5);

	priv->id = g_timeout_add (2, (GSourceFunc)SourceFunc, object);

	return object;
}

void
debugger_server_send_line (DebuggerServer *object, const gchar* line)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	g_assert (line != NULL);
	priv->out = g_list_append (priv->out, g_strdup (line));
}

void
debugger_server_stop (DebuggerServer *object)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	if (priv)
		priv->work = FALSE;
}

gchar*
debugger_server_get_line (DebuggerServer *object)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	gchar *ret = NULL;

	g_assert (priv->in != NULL);

	g_assert (priv->in->data != NULL);

	ret = g_strdup ((gchar*)priv->in->data);

	priv->in = g_list_delete_link (priv->in, priv->in);
	return ret;
}

gint
debugger_server_get_line_col (DebuggerServer *object)
{
	DebuggerServerPrivate *priv = DEBUGGER_SERVER_PRIVATE (object);
	return g_list_length (priv->in);
}
