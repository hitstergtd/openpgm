/* vim:ts=8:sts=4:sw=4:noai:noexpandtab
 * 
 * transport session ID helper functions
 *
 * Copyright (c) 2006-2009 Miru Limited.
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

#ifndef __PGM_TSI_H__
#define __PGM_TSI_H__

#include <glib.h>

typedef struct pgm_tsi_t pgm_tsi_t;

#ifndef __PGM_GSI_H__
#	include <pgm/gsi.h>
#endif

#ifndef __PGM_HASH_TABLE_H__
#	include <pgm/hashtable.h>
#endif


/* maximum length of TSI as a string */
#define PGM_TSISTRLEN		(sizeof("000.000.000.000.000.000.00000"))

struct pgm_tsi_t {
	pgm_gsi_t	gsi;		/* global session identifier */
	uint16_t	sport;		/* source port: a random number to help detect session re-starts */
};

G_BEGIN_DECLS

char* pgm_tsi_print (const pgm_tsi_t*) G_GNUC_WARN_UNUSED_RESULT;
int pgm_tsi_print_r (const pgm_tsi_t*, char*, size_t);
pgm_hash_t pgm_tsi_hash (const void*) G_GNUC_WARN_UNUSED_RESULT;
bool pgm_tsi_equal (const void* restrict, const void* restrict) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* __PGM_TSI_H__ */
