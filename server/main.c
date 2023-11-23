/*
 * Server main function
 *
 * Copyright (C) 1998 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif

#include <string.h>
#include <syslog.h>
#include <sys/types.h>

#include <time.h>

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

#include "object.h"
#include "file.h"
#include "thread.h"
#include "request.h"
#include "unicode.h"
#include "esync.h"

/* command-line options */
int debug_level = 0;
int foreground = 0;
timeout_t master_socket_timeout = 3 * -TICKS_PER_SEC;  /* master socket timeout, default is 3 seconds */
const char *server_argv0;
GMainLoop *loop;

/* parse-line args */

static void usage( FILE *fh )
{
    fprintf(fh, "Usage: %s [options]\n\n", server_argv0);
    fprintf(fh, "Options:\n");
    fprintf(fh, "   -d[n], --debug[=n]       set debug level to n or +1 if n not specified\n");
    fprintf(fh, "   -f,    --foreground      remain in the foreground for debugging\n");
    fprintf(fh, "   -h,    --help            display this help message\n");
    fprintf(fh, "   -k[n], --kill[=n]        kill the current wineserver, optionally with signal n\n");
    fprintf(fh, "   -p[n], --persistent[=n]  make server persistent, optionally for n seconds\n");
    fprintf(fh, "   -v,    --version         display version information and exit\n");
    fprintf(fh, "   -w,    --wait            wait until the current wineserver terminates\n");
    fprintf(fh, "\n");
}

static void parse_args( int argc, char *argv[] )
{
    int ret, optc;

    static struct option long_options[] =
    {
        {"debug",       2, NULL, 'd'},
        {"foreground",  0, NULL, 'f'},
        {"help",        0, NULL, 'h'},
        {"kill",        2, NULL, 'k'},
        {"persistent",  2, NULL, 'p'},
        {"version",     0, NULL, 'v'},
        {"wait",        0, NULL, 'w'},
        { NULL,         0, NULL, 0}
    };

    server_argv0 = argv[0];

    while ((optc = getopt_long( argc, argv, "d::fhk::p::vw", long_options, NULL )) != -1)
    {
        switch(optc)
        {
            case 'd':
                if (optarg && isdigit(*optarg))
                    debug_level = atoi( optarg );
                else
                    debug_level++;
                break;
            case 'f':
                foreground = 1;
                break;
            case 'h':
                usage(stdout);
                exit(0);
                break;
            case 'k':
                if (optarg && isdigit(*optarg))
                    ret = kill_lock_owner( atoi( optarg ) );
                else
                    ret = kill_lock_owner(-1);
                exit( !ret );
            case 'p':
                if (optarg && isdigit(*optarg))
                    master_socket_timeout = (timeout_t)atoi( optarg ) * -TICKS_PER_SEC;
                else
                    master_socket_timeout = TIMEOUT_INFINITE;
                break;
            case 'v':
                fprintf( stderr, "%s\n", PACKAGE_STRING );
                exit(0);
            case 'w':
                wait_for_lock();
                exit(0);
            default:
                usage(stderr);
                exit(1);
        }
    }
}

static void sigterm_handler( int signum )
{
    exit(1);  /* make sure atexit functions get called */
}

static void
on_name_appeared (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         user_data)
{
  GError *error;

  error = NULL;
  char userid[80];
  sprintf(userid, "%d", getuid());
  const gchar *reply;

  GVariant *result = g_dbus_connection_call_sync (connection,
                                        "kaka.HPServer",      /* bus name */
                                        "/kaka/HPObject",      /* object path */
                                        "kaka.HPInterface",             /* interface name */
                                        "MakeWineTempDir",                   /* method name */
                                        g_variant_new ("(s)", userid), /* parameters */
                                        G_VARIANT_TYPE ("(s)"),         /* return type */
                                        G_DBUS_CALL_FLAGS_NONE,
                                        G_MAXINT,
                                        NULL,
                                        &error);
  g_assert (result != NULL);
  g_variant_get (result, "(&s)", &reply);
  syslog(LOG_KERN,"%s\n",reply);
  g_main_loop_quit(loop);
}

static void
on_name_vanished (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_printerr ("Failed to get name owner for %s\n"
              "Is ./gdbus-example-server running?\n",
              name);
  g_main_loop_quit(loop);
}

int main( int argc, char *argv[] )
{
    guint watcher_id;

    watcher_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                    "kaka.HPServer",
                                    G_BUS_NAME_WATCHER_FLAGS_AUTO_START,
                                    on_name_appeared,
                                    on_name_vanished,
                                    NULL,
                                    NULL);

    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    g_bus_unwatch_name (watcher_id);

    setvbuf( stderr, NULL, _IOLBF, 0 );
    parse_args( argc, argv );

    /* setup temporary handlers before the real signal initialization is done */
    signal( SIGPIPE, SIG_IGN );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );
    signal( SIGQUIT, sigterm_handler );
    signal( SIGTERM, sigterm_handler );
    signal( SIGABRT, sigterm_handler );

    sock_init();
    open_master_socket();

    if (do_esync())
        esync_init();

    if (debug_level) fprintf( stderr, "wineserver: starting (pid=%ld)\n", (long) getpid() );
    set_current_time();
    init_scheduler();
    init_signals();
    init_directories( load_intl_file() );
    init_registry();
    main_loop();

    return 0;
}
