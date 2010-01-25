/***************************************************************************
 * Copyright (C) Maxim Ermilov 2009 <zaspire@rambler.ru>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>

#include <jsapi.h>
#include <jsscript.h>
#include <jscntxt.h>
#include <jsdbgapi.h>

#include <gio/gio.h>

#include "debug.h"

struct Breakpoint {
	GFile *file;
	guint lineno;
};

typedef struct {
	JSObject *obj;
	jsval name;
	jsval val;
} Value;

GList *Values = NULL;

gboolean wait_command = FALSE;
gint depth = 0;
gboolean stepover = FALSE;
gboolean stepout = FALSE;
GList *breakpoint = NULL;

/*Missed in .h but exported*/
extern JSContext *gjs_context_get_context(GjsContext * js_context);

static gchar *variable_get_desc(JSContext * cx, JSObject * obj);
static gchar *command_get();
static void send_reply(const gchar *a1);
static gchar *variable_get_local(JSContext * cx);

#define DEBUG_PRINT printf

static JSTrapStatus 
TrapHandler(JSContext * cx, JSScript * script,
			 jsbytecode * pc, jsval * rval, void *closure)
{
	gint tdepth;
	gboolean exec = FALSE;
	GList *i;
	GFile *file = NULL;
	gchar *file_path, *tmp;
	JSStackFrame *fi;
	JSStackFrame *fp;
	if (!script)
		return JSTRAP_CONTINUE;

	if (JS_GetScriptFilename(cx, script) == NULL)
		return JSTRAP_CONTINUE;

	file = g_file_new_for_path(JS_GetScriptFilename(cx, script));

	if (wait_command) {
		wait_command = FALSE;
		exec = TRUE;
	}
	tdepth = 0;
	fi = NULL;
	fp = JS_FrameIterator(cx, &fi);

	if (!fp)
		fp = JS_GetScriptedCaller(cx, NULL);
	while (fp) {
		if (JS_IsNativeFrame(cx, fp))
			break;
		tdepth++;
		fi = fp;
		fp = JS_GetScriptedCaller(cx, fp);
		/*Hack for Xulrunner 1.9.1*/
		if (fi == fp)
			break;
	}
	if (stepover) {
		if (tdepth <= depth) {
			exec = TRUE;
			stepover = FALSE;
		}
	}
	if (stepout) {
		if (tdepth < depth) {
			exec = TRUE;
			stepout = FALSE;
		}
	}
	for (i = breakpoint; i; i = g_list_next(i)) {
		struct Breakpoint *bp = (struct Breakpoint *)i->data;
		g_assert(bp);
		if (bp->lineno != JS_PCToLineNumber(cx, script, pc))
			continue;
		if (!g_file_equal(bp->file, file))
			continue;
		exec = TRUE;
		break;
	}
	if (!exec)
		return JSTRAP_CONTINUE;
	file_path = g_file_get_path(file);
	tmp = g_strdup_printf("Line #%d File:%s\n",
				    JS_PCToLineNumber(cx, script, pc),
				    file_path);
	g_free(file_path);
	send_reply(tmp);
	while (TRUE) {
		gboolean unknown_command = TRUE;
		gchar *str = command_get();
		JSTrapStatus result = 0xFF;

		g_assert(str != NULL);

		if (strcmp(str, "continue") == 0) {
			result = JSTRAP_CONTINUE;
			unknown_command = FALSE;
		}
		if (strcmp(str, "break") == 0) {
			result = JSTRAP_RETURN;
			unknown_command = FALSE;
		}
		if (strcmp(str, "stepin") == 0) {
			wait_command = TRUE;
			result = JSTRAP_CONTINUE;
			unknown_command = FALSE;
		}
		if (strcmp(str, "stepover") == 0) {
			depth = tdepth;
			stepover = TRUE;
			result = JSTRAP_CONTINUE;
			unknown_command = FALSE;
		}
		if (strcmp(str, "stepout") == 0) {
			depth = tdepth;
			stepout = TRUE;
			result = JSTRAP_CONTINUE;
			unknown_command = FALSE;
		}
		if (strcmp(str, "add") == 0) {
			struct Breakpoint *bp =
				    g_new(struct Breakpoint, 1);
			int line;
			gchar *com = command_get();
			g_assert(strlen(com) > 0);
			gchar *filename = g_malloc(strlen(com));
			if (sscanf (com, "%d %s", &line, filename) != 2) {
				g_warning("Invalid data");
				continue;
			}
			bp->lineno = line;
			bp->file = g_file_new_for_path (filename);
			breakpoint = g_list_append(breakpoint, bp);
			g_free(filename);
			g_free(com);
			unknown_command = FALSE;
		}
		if (strcmp(str, "eval") == 0) {
			gboolean work = TRUE;
			jsval rval;
			gchar *com = command_get();
			JSStackFrame *fi = 0;
			JSStackFrame *fp = JS_FrameIterator(cx, &fi);

			if (fp) {
				if (JS_EvaluateInStackFrame
					    (cx, fp, com, strlen(com),
					     "<debug_eval>", 1,
					     &rval) != JS_TRUE) {
					g_warning ("Can not eval;In stack");
					send_reply("ERROR");
				} else
					work = FALSE;
			} else {
				if (JS_EvaluateScript
					    (cx, JS_GetGlobalObject(cx),
					     com, strlen(com),
					     "<debug_eval>", 1,
					     &rval) != JS_TRUE) {
					g_warning ("Can not eval;");
					send_reply("ERROR");
				} else
					work = FALSE;
			}
			g_free(com);
			if (work)
				continue;
			if (JSVAL_IS_OBJECT(rval)) {
				JSObject *obj;
				JS_ValueToObject(cx, rval, &obj);
				gchar *str = variable_get_desc(cx, obj);
				send_reply(str);
				g_free(str);
			} else {
				gchar *name;
				JSString *str = JS_ValueToString(cx, rval);
				if (!str) {
					g_warning ("Exception in toString");
					send_reply ("ERROR");
					continue;
				}
				name = g_utf16_to_utf8(JS_GetStringChars
								(str), -1, NULL, NULL, NULL);
				send_reply(name);
				g_free(name);
			}
			unknown_command = FALSE;
		}
		if (strcmp(str, "desc") == 0) {
			gchar *str = variable_get_desc(cx, JS_GetGlobalObject(cx));
			send_reply(str);
			g_free(str);
			unknown_command = FALSE;
		}
		if (strcmp(str, "list") == 0) {
			gchar *str = variable_get_local(cx);
			send_reply(str);
			g_free (str);
			unknown_command = FALSE;
		}
		if (strcmp(str, "stacktrace") == 0) {
			gchar *ret = g_strdup(""), *t = NULL;

			JSStackFrame *fi = 0;
			JSStackFrame *fp = JS_FrameIterator(cx, &fi);
			if (!fp)
				fp = JS_GetScriptedCaller(cx, NULL);
			while (fp) {
				JSScript *s;
				jsbytecode *pc;
				int line;
				const gchar *name;
				if (JS_IsNativeFrame(cx, fp))
					break;
				s = JS_GetFrameScript(cx, fp);
				pc = JS_GetFramePC(cx, fp);
				if (!pc)
					break;
				line = JS_PCToLineNumber(cx, s, pc);
				name = JS_GetScriptFilename(cx, s);
				t = g_strdup_printf("%sLINE# %d %s, ", ret, 
								line, name);
				g_free(ret);
				ret = t;
				fi = fp;
				fp = JS_GetScriptedCaller(cx, fp);
				/*Hack for Xulrunner 1.9.1*/
				if (fi == fp)
					break;
			}
			send_reply(ret);
			g_free(ret);
			unknown_command = FALSE;
		}
		g_free(str);
		if (result != 0xFF)
			return result;
		g_assert(unknown_command != TRUE);
	}
	g_assert_not_reached();
}

static gchar *
variable_get_members (JSContext * cx, JSObject * obj)
{
	gchar *ret = g_strdup("");
	JSPropertyDescArray pda;
	JS_GetPropertyDescArray(cx, obj, &pda);
	uint32 i;

	for (i = 0; i < pda.length; i++) {
		JSString *str;
		gchar *name, *t;

		if (pda.array[i].flags & (JSPD_ERROR | JSPD_EXCEPTION))
			continue;
		if (!(pda.array[i].flags & (JSPD_VARIABLE | JSPD_ENUMERATE)))
			continue;

		str = JS_ValueToString(cx, pda.array[i].id);
		name = g_utf16_to_utf8(JS_GetStringChars(str), -1, NULL, NULL,
				    NULL);
		if (strcmp(name, "imports") == 0) {
			g_free(name);
			continue;
		}
		t = g_strconcat(ret, name, ",", NULL);
		g_free (ret);
		ret = t;

		g_free(name);
	}
	g_assert(ret != NULL);
	return ret;
}

static gchar *
variable_get_desc(JSContext * cx, JSObject * obj)
{
	gchar *ret = g_strdup("");
	JSPropertyDescArray pda;
	uint32 i;
	gboolean first = TRUE;

	if (!obj)
		return ret;

	JS_GetPropertyDescArray(cx, obj, &pda);

	for (i = 0; i < pda.length; i++) {
		JSString *str;
		gchar *name;
		gchar *t;
		const gchar *type;
		if (pda.array[i].flags & (JSPD_ERROR | JSPD_EXCEPTION))
			continue;

		str = JS_ValueToString(cx, pda.array[i].id);
		if (!str)
			continue;
		name = g_utf16_to_utf8(JS_GetStringChars(str), -1, NULL, NULL, NULL);

		type = JS_GetTypeName(cx, JS_TypeOfValue(cx, pda.array[i].value));
		if (first) {
			ret = g_strconcat("{ ", type, ", ", name, NULL);
			first = FALSE;
		} else {
			t = g_strconcat(ret, ", { ", type, ", ", name, NULL);
			g_free(ret);
			ret = t;
		}
		t = g_strconcat(ret, ",}", NULL);
		g_free(ret);
		ret = t;
		g_free(name);
	}
	g_assert(ret != NULL);
	return ret;
}

static gchar *
variable_get_local(JSContext * cx)
{
	JSObject *obj = NULL;
	JSStackFrame *fi = NULL;
	JSStackFrame *fp = NULL;

	fp = JS_FrameIterator(cx, &fi);
	if (fp) {
		obj = JS_GetFrameCallObject(cx, fp);
		if (!obj)
			obj = JS_GetFrameScopeChain(cx, fp);
	}
	if (!obj)
		obj = JS_GetGlobalObject(cx);

	g_assert (obj != NULL);

	return variable_get_members(cx, obj);
}

GList *to_send = NULL;
GMutex *command_mutex;
int command_has;
char *command = NULL;
int sock;

static void
send_reply(const gchar *a1)
{
	g_assert(a1 != NULL);
	to_send = g_list_append(to_send, g_strdup(a1));
}

static gchar *
command_get()
{
	gchar *ret = NULL;
	while (ret == NULL) {
		g_mutex_lock(command_mutex);
		if (to_send != NULL) {
			GList *iter;
			for (iter = to_send; iter; iter = g_list_next(iter)) {
				int size = strlen((char *)iter->data) + 1;
				if (send(sock, &size, 4, 0) == -1) {
					g_error("Can not send.");
				}
				if (send(sock, iter->data, size, 0) == -1) {
					g_error("Can not send.");
				}
			}
			g_list_foreach (to_send, (GFunc)g_free, NULL);
			g_list_free (to_send);
			to_send = NULL;
		}
		if (command_has) {
			ret = command;
			command_has--;
			g_mutex_unlock(command_mutex);
		} else {
			g_mutex_unlock(command_mutex);
			usleep(2);
		}
	}
	return ret;
}

static void
NewScriptHook(JSContext * cx, const char *filename, uintN lineno,
		   JSScript * script, JSFunction * fun, void *callerdata)
{
	uintN i;
	uintN col;
	if (strstr(filename, ".js") == NULL || strncmp(filename, "/usr/", 5) == 0 || !script)
		return;
	DEBUG_PRINT("New Script : %s\n", filename);
	col = JS_PCToLineNumber(cx, script, script->code + script->length - 1);
	for (i = 0; i <= col; i++) {
		JS_SetTrap(cx, script, JS_LineNumberToPC(cx, script, i),
			   TrapHandler, NULL);
	}
}

static gpointer
debug_thread(gpointer data)
{
	while (1) {
		int len;
		gchar *buf = NULL;
		while (command_has != 0)
			usleep(2);
		g_mutex_lock(command_mutex);
		if (ioctl(sock, FIONREAD, &len) == -1) {
			g_error("Error in ioctl call.");
		}
		if (len > 4) {
			if (recv(sock, &len, 4, 0) == -1) {
				g_error("Can not recv.");
			}
			g_assert(len > 0);
			buf = g_malloc(len + 1);
			if (recv(sock, buf, len, 0) == -1) {
				g_error("Can not recv.");
			}
			buf[len] = '\0';
			command = g_strdup(buf);
			g_free(buf);
			command_has++;
		}
		g_mutex_unlock(command_mutex);
	}
}

void
debug_init(int *argc, char ***argv, GjsContext * context)
{
	int i;
	int flag;
   int port = 1235;
	struct in_addr sip;
	struct sockaddr_in ssa;
	JSContext *cx = gjs_context_get_context(context);
	JSRuntime *rt = JS_GetRuntime(cx);
	gchar *ip = NULL;
	for (i = 0; i < *argc - 1; i++) {
		if (strcmp((*argv)[i], "--debug") == 0) {
			ip = (*argv)[i + 1];
			wait_command = TRUE;
		}
		if (strcmp((*argv)[i], "--js-port") == 0) {
			sscanf((*argv)[i + 1], "%d", &port);
		}
	}

   g_assert (port > 0);

	if (ip == NULL)
		return;
	if (!g_thread_supported())
		g_thread_init(NULL);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (inet_aton(ip, &sip) != 1) {
		g_error("Incorrect addres.");
	}
	ssa.sin_family = AF_INET;
	ssa.sin_port = htons(port);
	ssa.sin_addr = sip;
	flag = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
	if (connect(sock, (struct sockaddr *)&ssa, sizeof(ssa)) == -1) {
		g_error("Can not connect.");
	}

	command_mutex = g_mutex_new();
	command_has = 0;
	g_thread_create(debug_thread, NULL, FALSE, NULL);
	JS_SetNewScriptHook(rt, NewScriptHook, NULL);
}
