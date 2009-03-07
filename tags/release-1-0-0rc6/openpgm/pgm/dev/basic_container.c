/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * Test basic containers for alloc performance.
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
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>


#include <glib.h>


struct tests {
	double	      (*test_func)(int, int);
	const char*	name;
};


/* globals */

double test_control (int, int);
double test_alloc_list (int, int);
double test_alloc_list_malloc (int, int);
double test_alloc_list_stack (int, int);
double test_alloc_slist (int, int);
double test_alloc_queue (int, int);
double test_alloc_hash (int, int);
double test_alloc_ptr_array (int, int);
double test_alloc_byte_array (int, int);

#if HAVE_GLIB_SEQUENCE
double test_alloc_sequence (int, int);
#endif

G_GNUC_NORETURN static void
usage (
	const char*	bin
	)
{
	fprintf (stderr, "Usage: %s [options]\n", bin);
	exit (1);
}

int
main (
	int		argc,
	char*		argv[]
	)
{
	puts ("basic_container");

/* parse program arguments */
	const char* binary_name = strrchr (argv[0], '/');
	int c;
	while ((c = getopt (argc, argv, "h")) != -1)
	{
		switch (c) {

		case 'h':
		case '?': usage (binary_name);
		}
	}

/* setup signal handlers */
	signal(SIGHUP, SIG_IGN);

	int test_size[] = { 100000, 200000, 100000, 200000, 0 };
	int test_payload[] = { /*9000,*/ 1500, 0 };
	struct tests tests[] = {
//			{ test_control, "control" },
//			{ test_alloc_list_malloc, "list/malloc" },
//			{ test_alloc_list, "list/slice" },
			{ test_alloc_list_stack, "list/stack" },
			{ test_alloc_slist, "slist" },
			{ test_alloc_hash, "hash" },
			{ test_alloc_queue, "queue" },
			{ test_alloc_ptr_array, "*array" },
			{ test_alloc_byte_array, "byte array" },
#if HAVE_GLIB_SEQUENCE
			{ test_alloc_sequence, "sequence" },
#endif
			{ NULL, NULL }
			};

/* print header */
	struct tests* p;
	int* p2;
	int* p3;

	int test_count = 3;

	p = tests;
	do {
		p2 = test_payload;
		do {
			for (int test = 1; test <= test_count; test++)
			{
				printf (",%s@%ib/%i", p->name, *p2, test);
			}
		} while (*(++p2));
	} while ((++p)->name);
	putchar ('\n');

/* each row is one payload size */
	p3 = test_size;
	do {
		printf ("%i", *p3);

		p = tests;
		do {
			p2 = test_payload;
			do {
				for (int test = 1; test <= 3; test++)
				{
					printf (",%g", p->test_func (*p3, *p2) );
				}
			} while (*(++p2));
		} while ((++p)->name);

		putchar ('\n');
	} while (*(++p3));

	puts ("finished.");
	return 0;
}

static void
_list_iterator (
	gpointer	data,
	gpointer	user_data
	)
{
	if ( *(int*)user_data )
		g_slice_free1 ( *(int*)user_data, data );
}

static void
_list_free_iterator (
	gpointer	data,
	gpointer	user_data
	)
{
	if ( *(int*)user_data )
		free (data);
}

double
test_control (
	G_GNUC_UNUSED int count,
	G_GNUC_UNUSED int size_per_entry
	)
{
#if 1
	char *p = g_slice_alloc (100);
	g_slice_free1 (100, p);
#else
	char *p = malloc (100);
	free (p);
#endif

	return 0.0;
}

double
test_alloc_list (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GList *list = NULL;
	int i;

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_slice_alloc (size_per_entry) : NULL;
		list = g_list_prepend (list, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_list_foreach (list, _list_iterator, &size_per_entry);
	g_list_free (list);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_list_malloc (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GList *list = NULL;

	gettimeofday(&start, NULL);
	for (int i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_malloc (size_per_entry) : NULL;
		list = g_list_prepend (list, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_list_foreach (list, _list_free_iterator, &size_per_entry);
	g_list_free (list);
	list = NULL;

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_list_stack (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GList *list = NULL;
	GTrashStack *stack = NULL;
	int i;

	if (size_per_entry)
	{
		for (i = 0; i < count; i++)
		{
			char *entry = g_slice_alloc (size_per_entry);
			g_trash_stack_push (&stack, entry);
		}
	}

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_trash_stack_pop (&stack) : NULL;
		list = g_list_prepend (list, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_list_foreach (list, _list_iterator, &size_per_entry);
	g_list_free (list);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_slist (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GSList *list = NULL;
	GTrashStack *stack = NULL;
	int i;

	if (size_per_entry)
	{
		for (i = 0; i < count; i++)
		{
			char *entry = g_slice_alloc (size_per_entry);
			g_trash_stack_push (&stack, entry);
		}
	}

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_trash_stack_pop (&stack) : NULL;
		list = g_slist_prepend (list, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_slist_foreach (list, _list_iterator, &size_per_entry);
	g_slist_free (list);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_queue (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GQueue* queue = NULL;
	GTrashStack *stack = NULL;
	int i;

	if (size_per_entry)
	{
		for (i = 0; i < count; i++)
		{
			char *entry = g_slice_alloc (size_per_entry);
			g_trash_stack_push (&stack, entry);
		}
	}

	queue = g_queue_new ();

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_trash_stack_pop (&stack) : NULL;
		g_queue_push_head (queue, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_queue_foreach (queue, _list_iterator, &size_per_entry);
	g_queue_free (queue);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

#if HAVE_GLIB_SEQUENCE
double
test_alloc_sequence (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GSequence* sequence = NULL;
	GTrashStack *stack = NULL;
	int i;

	if (size_per_entry)
	{
		for (i = 0; i < count; i++)
		{
			char *entry = g_slice_alloc (size_per_entry);
			g_trash_stack_push (&stack, entry);
		}
	}

	sequence = g_sequence_new (NULL);

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_trash_stack_pop (&stack) : NULL;
		g_sequence_append (p, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_sequence_foreach (sequence, _list_iterator, &size_per_entry);
	g_sequence_free (sequence);

	return (secs * 1000.0 * 1000.0) / (double)count;
}
#endif

static void
_hash_iterator (
	G_GNUC_UNUSED gpointer key,
	gpointer	value,
	gpointer	user_data
	)
{
	g_slice_free1 ( *(int*)user_data + sizeof(int), value );
}

double
test_alloc_hash (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GHashTable* hash = NULL;
	GTrashStack *stack = NULL;
	int i;

	for (i = 0; i < count; i++)
	{
/* we add an integer to every datum for use as the hash key */
		char *entry = g_slice_alloc (size_per_entry + sizeof(int));
		g_trash_stack_push (&stack, entry);
	}

	hash = g_hash_table_new (g_int_hash, g_int_equal);

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = g_trash_stack_pop (&stack);
		int *key = (int*)entry;
		*key = i;
		g_hash_table_insert (hash, key, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_hash_table_foreach (hash, _hash_iterator, &size_per_entry);
	g_hash_table_destroy (hash);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_ptr_array (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GPtrArray* array = NULL;
	GTrashStack *stack = NULL;
	int i;

	if (size_per_entry)
	{
		for (i = 0; i < count; i++)
		{
			char *entry = g_slice_alloc (size_per_entry);
			g_trash_stack_push (&stack, entry);
		}
	}

	array = g_ptr_array_new ();

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		char *entry = size_per_entry ? g_trash_stack_pop (&stack) : NULL;
		g_ptr_array_add (array, entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	if (size_per_entry)
		g_ptr_array_foreach (array, _list_iterator, &size_per_entry);
	g_ptr_array_free (array, TRUE);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

double
test_alloc_byte_array (
	int		count,
	int		size_per_entry
	)
{
	struct timeval start, now;
	GByteArray* array = NULL;
	guint8 *data = NULL;
	int i;

	array = g_byte_array_new ();
	data = size_per_entry ? g_slice_alloc (size_per_entry) : NULL;

	gettimeofday(&start, NULL);
	for (i = 0; i < count; i++)
	{
		g_byte_array_append (array, data, size_per_entry);
	}
	gettimeofday(&now, NULL);

        double secs = (now.tv_sec - start.tv_sec) + ( (now.tv_usec - start.tv_usec) / 1000.0 / 1000.0 );

	g_byte_array_free (array, TRUE);
	if (size_per_entry)
		g_slice_free1 (size_per_entry, data);

	return (secs * 1000.0 * 1000.0) / (double)count;
}

/* eof */
