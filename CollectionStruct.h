/***********************************************************
        Copyright 1991 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

CMU DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
CMU BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.
******************************************************************/

#ifndef _COLLECTION_STRUCT_H
#define _COLLECTION_STRUCT_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/CollectionStruct.h,v 4.4 1992/06/19 20:20:43 ww0r Exp $ */


/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#ifdef USE_FSINFO
typedef struct collectionfsinfo
{
  char		*path;		/* path to mountpoint */
  long		fs_id;		/* volume no. */
  u_long	fs_modtime;	/* volume mod time */
} COLLECTIONFSINFO;
#endif USE_FSINFO

typedef struct collection
{
  char		*name;	/* name of the collection */
  char		*path;	/* location of the collection in the file system */
  DEPOTDB	*info;	/* db tree containing info on files in collection */
#ifdef USE_FSINFO
  COLLECTIONFSINFO	**fslist;	/* NULL-terminated array of file system mount points */
  COLLECTIONFSINFO	*confinfo;
  Boolean	fsuptodate;	/* set to FALSE unless all the file systems under collection and the config file have been unchanged since the last run */
#endif USE_FSINFO
} COLLECTION;

typedef struct collectionlist
{
  struct collectionlist	*next;
  COLLECTION		collection;	/* item in collectionlist */
} COLLECTIONLIST;

#define COLLECTION_next(x) ((x)->next)

#endif /* _COLLECTION_STRUCT_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/CollectionStruct.h,v $ */
