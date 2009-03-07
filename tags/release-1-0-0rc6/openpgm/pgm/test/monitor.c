/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * PGM link monitor.
 *
 * Copyright (c) 2006-2007 Miru Limited.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <glib.h>

#include <pgm/backtrace.h>
#include <pgm/signal.h>
#include <pgm/log.h>
#include <pgm/packet.h>


/* globals */

static int g_port = 7500;
static const char* g_network = "239.192.0.1";
static struct in_addr g_filter = { 0 };

static GIOChannel* g_io_channel = NULL;
static GIOChannel* g_stdin_channel = NULL;
static GMainLoop* g_loop = NULL;


static void on_signal (int);
static gboolean on_startup (gpointer);
static gboolean on_mark (gpointer);

static gboolean on_io_data (GIOChannel*, GIOCondition, gpointer);
static gboolean on_io_error (GIOChannel*, GIOCondition, gpointer);

static gboolean on_stdin_data (GIOChannel*, GIOCondition, gpointer);

int
main (
	G_GNUC_UNUSED int	argc,
	G_GNUC_UNUSED char   *argv[]
	)
{
	puts ("monitor");

	log_init ();

/* setup signal handlers */
	signal(SIGSEGV, on_sigsegv);
	pgm_signal_install(SIGINT, on_signal);
	pgm_signal_install(SIGTERM, on_signal);
	pgm_signal_install(SIGHUP, SIG_IGN);

/* delayed startup */
	puts ("scheduling startup.");
	g_timeout_add(0, (GSourceFunc)on_startup, NULL);

/* dispatch loop */
	g_loop = g_main_loop_new(NULL, FALSE);

	puts ("entering main event loop ... ");
	g_main_loop_run(g_loop);

	puts ("event loop terminated, cleaning up.");

/* cleanup */
	g_main_loop_unref(g_loop);
	g_loop = NULL;

	if (g_io_channel) {
		puts ("closing socket.");

		GError *err = NULL;
		g_io_channel_shutdown (g_io_channel, FALSE, &err);
		g_io_channel = NULL;
	}

	if (g_stdin_channel) {
		puts ("unbinding stdin.");
		g_io_channel_unref (g_stdin_channel);
		g_stdin_channel = NULL;
	}

	puts ("finished.");
	return 0;
}

static void
on_signal (
	G_GNUC_UNUSED int	signum
	)
{
	puts ("on_signal");

	g_main_loop_quit(g_loop);
}

static gboolean
on_startup (
	G_GNUC_UNUSED gpointer data
	)
{
	int e;

	puts ("startup.");

/* find PGM protocol id */
// TODO: fix valgrind errors
	int ipproto_pgm = IPPROTO_PGM;
#if HAVE_GETPROTOBYNAME_R
	char b[1024];
	struct protoent protobuf, *proto;
	e = getprotobyname_r("pgm", &protobuf, b, sizeof(b), &proto);
	if (e != -1 && proto != NULL) {
		if (proto->p_proto != ipproto_pgm) {
			printf("Setting PGM protocol number to %i from /etc/protocols.\n");
			ipproto_pgm = proto->p_proto;
		}
	}
#else
	struct protoent *proto = getprotobyname("pgm");
	if (proto != NULL) {
		if (proto->p_proto != ipproto_pgm) {
			printf("Setting PGM protocol number to %i from /etc/protocols.\n", proto
->p_proto);
			ipproto_pgm = proto->p_proto;
		}
	}
#endif

/* open socket for snooping */
	puts ("opening raw socket.");
	int sock = socket(PF_INET, SOCK_RAW, ipproto_pgm);
	if (sock < 0) {
		int _e = errno;
		perror("on_startup() failed");

		if (_e == EPERM && 0 != getuid()) {
			puts ("PGM protocol requires this program to run as superuser.");
		}
		g_main_loop_quit(g_loop);
		return FALSE;
	}

/* drop out of setuid 0 */
	if (0 == getuid()) {
		puts ("dropping superuser privileges.");
		setuid((gid_t)65534);
		setgid((uid_t)65534);
	}

	char _t = 1;
	e = setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &_t, sizeof(_t));
	if (e < 0) {
		perror("on_startup() failed");
		close(sock);
		g_main_loop_quit(g_loop);
		return FALSE;
	}

/* buffers */
	int buffer_size = 0;
	socklen_t len = 0;
	e = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffer_size, &len);
	if (e == 0) {
		printf ("receive buffer set at %i bytes.\n", buffer_size);
	}
	e = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffer_size, &len);
	if (e == 0) {
		printf ("send buffer set at %i bytes.\n", buffer_size);
	}

/* bind */
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	e = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (e < 0) {
		perror("on_startup() failed");
		close(sock);
		g_main_loop_quit(g_loop);
		return FALSE;
	}

/* multicast */
	struct ip_mreqn mreqn;
	memset(&mreqn, 0, sizeof(mreqn));
	mreqn.imr_address.s_addr = htonl(INADDR_ANY);
	printf ("listening on interface %s.\n", inet_ntoa(mreqn.imr_address));
	mreqn.imr_multiaddr.s_addr = inet_addr(g_network);
	printf ("subscription on multicast address %s.\n", inet_ntoa(mreqn.imr_multiaddr));
	e = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreqn, sizeof(mreqn));
	if (e < 0) {
		perror("on_startup() failed");
		close(sock);
		g_main_loop_quit(g_loop);
		return FALSE;
	}

/* multicast loopback */
/* multicast ttl */

/* add socket to event manager */
	g_io_channel = g_io_channel_unix_new (sock);
	printf ("socket opened with encoding %s.\n", g_io_channel_get_encoding(g_io_channel));

	/* guint event = */ g_io_add_watch (g_io_channel, G_IO_IN | G_IO_PRI, on_io_data, NULL);
	/* guint event = */ g_io_add_watch (g_io_channel, G_IO_ERR | G_IO_HUP | G_IO_NVAL, on_io_error, NULL);

/* add stdin to event manager */
	g_stdin_channel = g_io_channel_unix_new (fileno(stdin));
	printf ("binding stdin with encoding %s.\n", g_io_channel_get_encoding(g_stdin_channel));

	g_io_add_watch (g_stdin_channel, G_IO_IN | G_IO_PRI, on_stdin_data, NULL);

/* period timer to indicate some form of life */
// TODO: Gnome 2.14: replace with g_timeout_add_seconds()
	g_timeout_add(10 * 1000, (GSourceFunc)on_mark, NULL);

	puts ("READY");
	fflush (stdout);
	return FALSE;
}

static gboolean
on_mark (
	G_GNUC_UNUSED gpointer data
	)
{
	g_message ("-- MARK --");
	return TRUE;
}

static gboolean
on_io_data (
	GIOChannel* source,
	G_GNUC_UNUSED GIOCondition condition,
	G_GNUC_UNUSED gpointer data
	)
{
	char buffer[4096];
	int fd = g_io_channel_unix_get_fd(source);
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(addr);
	int len = recvfrom(fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&addr, &addr_len);

	if (g_filter.s_addr && g_filter.s_addr != addr.sin_addr.s_addr) {
		return TRUE;
	}

	printf ("%i bytes received from %s.\n", len, inet_ntoa(addr.sin_addr));

	monitor_packet (buffer, len);
	fflush (stdout);

	return TRUE;
}

static gboolean
on_io_error (
	GIOChannel* source,
	G_GNUC_UNUSED GIOCondition condition,
	G_GNUC_UNUSED gpointer data
	)
{
	puts ("on_error.");

	GError *err;
	g_io_channel_shutdown (source, FALSE, &err);

/* remove event */
	return FALSE;
}

/* process input commands from stdin/fd 
 */

static gboolean
on_stdin_data (
	GIOChannel* source,
	G_GNUC_UNUSED GIOCondition condition,
	G_GNUC_UNUSED gpointer data
	)
{
	gchar* str = NULL;
	gsize len = 0;
	gsize term = 0;
	GError* err = NULL;

	g_io_channel_read_line (source, &str, &len, &term, &err);
	if (len > 0) {
		if (term) str[term] = 0;

		if (strcmp(str, "quit") == 0) {
			g_main_loop_quit(g_loop);
		} else if (strncmp(str, "filter ", strlen("filter ")) == 0) {
			unsigned a, b, c, d;
			int retval = sscanf(str, "filter %u.%u.%u.%u", &a, &b, &c, &d);
			if (retval == 4) {
				g_filter.s_addr = (d << 24) | (c << 16) | (b << 8) | a;
				puts ("READY");
			} else {
				printf ("invalid syntax for filter command.");
			}
		} else {
			printf ("unknown command: %s\n", str);
		}
	}

	fflush (stdout);
	g_free (str);
	return TRUE;
}

/* eof */
