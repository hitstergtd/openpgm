/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * portable hash table.
 *
 * Copyright (c) 2010 Miru Limited.
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

#ifndef __PGM_HASHTABLE_H__
#define __PGM_HASHTABLE_H__

#include <stdbool.h>
#include <stdint.h>
#include <glib.h>


typedef struct pgm_hashtable_t pgm_hashtable_t;
typedef uint_fast32_t pgm_hash_t;

typedef pgm_hash_t (*PGMHashFunc) (gconstpointer);
typedef bool (*PGMEqualFunc) (gconstpointer, gconstpointer);


G_BEGIN_DECLS

pgm_hashtable_t* pgm_hash_table_new (PGMHashFunc, PGMEqualFunc);
void pgm_hash_table_destroy (pgm_hashtable_t*);
void pgm_hash_table_insert (pgm_hashtable_t*, gconstpointer, gpointer);
bool pgm_hash_table_remove (pgm_hashtable_t*, gconstpointer);
void pgm_hash_table_remove_all (pgm_hashtable_t*);
gpointer pgm_hash_table_lookup (pgm_hashtable_t*, gconstpointer);

void pgm_hash_table_unref (pgm_hashtable_t*);

/* Hash Functions
 */
bool pgm_str_equal (const void* restrict, const void* restrict) G_GNUC_WARN_UNUSED_RESULT;
pgm_hash_t pgm_str_hash (const void*) G_GNUC_WARN_UNUSED_RESULT;
bool pgm_int_equal (const void* restrict, const void* restrict) G_GNUC_WARN_UNUSED_RESULT;
pgm_hash_t pgm_int_hash (const void*) G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* __PGM_HASHTABLE_H__ */