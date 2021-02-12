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

#ifndef  _COLLECTION_H
#define _COLLECTION_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Collection.h,v 4.5 1992/06/19 20:19:59 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

/* update specifications in config file for collection */
#define CU_MAP		001
#define CU_RCFILE	002
#define CU_DELETE	004

#ifdef USE_FSINFO
#define CU_AFSMOUNTPOINT 010
#endif USE_FSINFO

extern Boolean Collection_Read();
extern char *Collection_LocatePath();
extern Boolean Collection_Uptodate();

extern COLLECTIONLIST *Depot_CollectionList;

extern COLLECTIONLIST *CollectionList_GetAllCollections();
extern COLLECTIONLIST *CollectionList_GetCollectionList();
extern COLLECTIONLIST *CollectionList_GetAllKnownCollections();
extern COLLECTIONLIST *CollectionList_GetKnownCollections();
extern void CollectionList_FreeList();

#endif /* COLLECTION_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/Collection.h,v $ */
