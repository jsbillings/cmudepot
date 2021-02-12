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

#ifndef _DEPOTDBVERSION1_H
#define _DEPOTDBVERSION1_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBVersion1.h,v 4.4 1992/06/19 20:32:37 ww0r Exp $ */


/*
 * Author: Sohan C. Ramakrishna Pillai
 */


extern DEPOTDB *DepotDB1_Create();
extern int	DepotDB1_GetVersionNumber();
extern DEPOTDB *DepotDB1_Read();
extern Boolean	DepotDB1_Write();
extern DEPOTDB *DepotDB1_Append();
extern Boolean	DepotDB1_Apply();
extern DEPOTDB *DepotDB1_RemoveCollection();
extern DEPOTDB *DepotDB1_ObsoleteCollection();
extern DEPOTDB *DepotDB1_PruneCollection();
extern void	DepotDB1_Antiquate();
extern void	DepotDB1_ProtectSpecialFiles();
extern void	DepotDB1_SetTargetMappings();
extern DEPOTDB *DepotDB1_UpdateEntry();
extern ENTRY   *DepotDB1_LocateEntryByName();
extern DEPOTDB *DepotDB1_DeletePath();
extern DEPOTDB *DepotDB1_SetNonVirginPath();
extern void	DepotDB1_SelfReferenceRoot();
extern void	DepotDB1_FreeSourceList();
extern void	DepotDB1_SourceList_AddSource();
extern SOURCE  *DepotDB1_SourceList_LocateCollectionSourceByName();
extern Boolean	DepotDB1_AntiqueEntry();

#endif /* _DEPOTDBVERSION1_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotDBVersion1.h,v $ */
