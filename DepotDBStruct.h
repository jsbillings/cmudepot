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

#ifndef _DEPOTDBSTRUCT_H
#define _DEPOTDBSTRUCT_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBStruct.h,v 4.4 1992/06/19 20:23:10 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */


typedef struct source
{
  char		*name;	/* name of source file/directory */
  u_short	update_spec;	/* update specification for target file/dir */
  u_short	old_update_spec;	/* update specification on previous run of depot */
  char		*collection_name;	/* name of collection to which source belongs */
  unsigned	status;		/* flag indicating status of source */
#ifdef USE_FSINFO
  unsigned	fs_status;	/* FS_UNKNOWN|FS_INHERIT|FS_NEWFILESYSTEM */
  long		fs_id;		/* volume no. */
  u_long	fs_modtime;	/* volume mod time */
  long		col_id;		/* collection's volume no. */
  u_long	col_conftime;	/* mod time of collection's config info */
  				/* same as collection's mod time if no config info */
#endif USE_FSINFO
} SOURCE;


typedef struct entry
{
  struct entry *sibling;	/* next entry in the same directory */
  struct entry *child;       	/* entry for a child, if this is a directory */
  char		*name;		/* name of the file/directory */
  SOURCE	*sourcelist;	/* list of possible sources for file/dir */
  u_short	nsources;	/* number of possible sources for file/dir */
  unsigned	status;		/* flag indicating status of entry */
#define ENTRY_sibling(x)	((x)->sibling)
#define ENTRY_child(x)		((x)->child)
} ENTRY, DEPOTDB;

#endif /* _DEPOTDBSTRUCT_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBStruct.h,v $ */
