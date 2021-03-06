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

#ifndef _ANDREWFILESYSTEM_H
#define _ANDREWFILESYSTEM_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/AndrewFileSystem.h,v 4.4 1992/06/19 20:16:51 ww0r Exp $ */


/*
 * Author: Sohan C. Ramakrishna Pillai
 */


extern void AFS_GetVolumeInfo();
extern void AFS_GetMountPointInfo();
extern int AFS_vol_init();
extern void AFS_vol_done();

#endif /* _ANDREWFILESYSTEM_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/AndrewFileSystem.h,v $ */
