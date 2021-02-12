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

#ifndef _DEPOTERRORCODES_H
#define _DEPOTERRORCODES_H

/* $Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotErrorCodes.h,v 4.10 1992/06/19 20:19:09 ww0r Exp $ */

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

/* exit status codes for the depot program */
#define E_BADOPTION	1
#define E_BADVERSION	2
#define E_NULLTARGET	3

#define E_OUTOFMEMORY	11
#define E_FOPENFAILED	12
#define E_EMPTYDBFILE	13
#define E_BADUPDTFLAG	14
#define E_NULLDATABASE	15
#define E_NULLENTRY	16
#define E_UNNAMEDENTRY	17
#define E_SRCLESSENTRY	18
#define E_UNNAMEDSRC	19
#define E_UNREFSOURCE	20
#define E_BADENTRY	21
#define E_STATFAILED	26
#define E_LSTATFAILED	27
#define E_FSTATFAILED	28
#define E_OPENFAILED	29
#define E_NOENTRY	30
#define E_NOTDIR	31
#define E_BADUSER	34
#define E_BADLOCKFILE	35
#define E_NOLOCK	36
#define E_FOOLPROOFLOCK	37
#define E_CANTLOCK	38
#define E_BADSRCUPDT	39

#define E_LINKFAILED	40
#define E_OPENDIRFAILED	41
#define E_MKDIRFAILED	42
#define E_RMDIRFAILED	43
#define E_RDLINKFAILED	44
#define E_UNLINKFAILED	45
#define E_EXECFAILED	46
#define E_WRITEFAILED	47
#define E_VFORKFAILED	48
#define E_RENAMEFAILED	49
#define E_BADCOMMAND	50
#define E_BADCONFIGFILE	51
#define E_UNNAMEDCOLLISTFILE 52
#define E_BADCOLLISTFILE 53
#define E_UNKNOWNCOLLECTION 54

#define E_BADPREFFILE	70
#define E_BADPREFERENCE	71
#define E_NULLSTRARRAY	72
#define E_NULLSTRING	73
#define E_PREFSETFAILED	75
#define E_INCOMPLETEPREFINFO	76

#define E_BADDESTFILE	79
#define E_BADSRCFILE	80
#define E_NULLSEARCHPATH 81
#define	E_TYPECONFLICT	82
#define E_NEWDBEXISTS	83
#define E_DBBACKFAILED	84
#define E_DBUPDTFAILED	85
#define E_BADEXECFILE	86
#define E_NOEXECFILE	87
#define E_NULLCMDSOURCE	88
#define E_NULLCMDLABEL	89

#define E_NOFSINFO	90
#define E_INCOMPLETEFSINFO 91
#define E_UNLISTEDMTPT	92
#define E_BADMTPT	93
#define E_BADVLDBINIT	94
#define E_BADVLDBFETCH	95
#define E_BADVOLUME	96
#define E_AFSPROBLEM	97

#define DE_CHOWNFAILED	101
#define DE_CHMODFAILED	102
#ifdef USE_UTIME
#define DE_UTIMEFAILED	103
#else /* USE_UTIME */
#define DE_UTIMESFAILED	103
#endif /* USE_UTIME */

typedef struct depoterror
{
  int status;
  char *description;
} DEPOTERROR;

static DEPOTERROR DepotErrorList[] =
{
  { E_BADOPTION,	"Bad command line option" },
  { E_BADVERSION,	"Bad database file version" },
  { E_NULLTARGET,	"No target specified" },
  { E_OUTOFMEMORY,	"Memory allocation failed - out of memory" },
  { E_FOPENFAILED,	"Call to fopen(3) failed" },
  { E_EMPTYDBFILE,	"Empty database file" },
  { E_BADUPDTFLAG,	"Unknown update specification in database file" },
  { E_NULLDATABASE,	"Database operation attempted on a NULL database" },
  { E_NULLENTRY,	"Attempt to access non-existent entry in a database" },
  { E_UNNAMEDENTRY,	"Entry with no name found in a database" },
  { E_SRCLESSENTRY,	"Entry with no source found in a database" },
  { E_UNNAMEDSRC,	"Source with no name found in a database" },
  { E_UNREFSOURCE,	"Source with no collection to reference found in a database" },
  { E_BADENTRY,		"Source from expected collection not found" },
  { E_STATFAILED,	"Call to stat(2) failed" },
  { E_LSTATFAILED,	"Call to lstat(2) failed" },
  { E_FSTATFAILED,	"Call to fstat(2) failed" },
  { E_OPENFAILED,	"Call to open(2) failed" },
  { E_NOENTRY,		"Expected entry not found in a database" },
  { E_NOTDIR,		"Expected directory was not found or is not a directory" },
  { E_BADUSER,		"Depot operation invoked by unknown user" },
  { E_BADLOCKFILE,	"Attempt to query lock failed" },
  { E_NOLOCK,		"Lock operation failed" },
  { E_FOOLPROOFLOCK,	"Attempt to pick a lock failed" },
  { E_CANTLOCK,		"Lock operation failed" },
  { E_BADSRCUPDT,	"Unresolvable conflicting sources" },
  { E_LINKFAILED,	"Call to symlink(2) failed" },
  { E_OPENDIRFAILED,	"Call to opendir(3) failed" },
  { E_MKDIRFAILED,	"Call to mkdir(2) failed" },
  { E_RMDIRFAILED,	"Call to rmdir(2) failed" },
  { E_RDLINKFAILED,	"Call to readlink(2) failed" },
  { E_UNLINKFAILED,	"Call to unlink(2) failed" },
  { E_EXECFAILED,	"Command execution exited abnormally" },
  { E_WRITEFAILED,	"Call to write(2) failed" },
  { E_VFORKFAILED,	"Call to vfork(2) failed" },
  { E_RENAMEFAILED,	"Call to rename(2) failed" },
  { E_BADCOMMAND,	"Unknown tilde command in collection configuration file" },
  { E_BADCONFIGFILE,	"Syntax error in collection configuration file" },
  { E_UNNAMEDCOLLISTFILE,	"File with list of collections not specified" },
  { E_BADCOLLISTFILE,	"Syntax error in collection list file" },
  { E_UNKNOWNCOLLECTION, "Attempted operation on unknown collection" },
  { E_BADPREFFILE,	"Syntax error in customization file" },
  { E_BADPREFERENCE,	"Unknown preference type" },
  { E_NULLSTRARRAY,	"Attempted string-array operation forbidden on NULL string array" },
  { E_NULLSTRING,	"Attempted string-array operation forbidden on NULL string" },
  { E_PREFSETFAILED,	"Attempt to set a preference value failed" },
  { E_INCOMPLETEPREFINFO,	"Incomplete preference information in database file" },
  { E_BADDESTFILE,	"File to be updated is not writeable" },
  { E_BADSRCFILE,	"Could not access expected source file" },
  { E_NULLSEARCHPATH,	"NULL searchpath found for collection" },
  { E_TYPECONFLICT,	"Unresolvable conflicting sources" },
  { E_NEWDBEXISTS,	"Previous depot operation exited abnormally - repair required" },
  { E_DBBACKFAILED,	"Attempt to back up old database file failed" },
  { E_DBUPDTFAILED,	"Attempt to update database file failed" },
  { E_BADEXECFILE,	"Expected command file is not executable" },
  { E_NOEXECFILE,	"Expected executable file does not exist" },
  { E_NULLCMDSOURCE,	"Expected command source is NULL" },
  { E_NULLCMDLABEL,	"Expected command label is NULL" },
  { E_NOFSINFO,		"No file system information found for mountpoint in database file" },
  { E_INCOMPLETEFSINFO,	"Incomplete file system information found for mountpoint in database file" },
  { E_UNLISTEDMTPT,	"File system mountpoint found which was not specified in collection configuration file" },
  { E_BADMTPT,		"Bad file system mountpoint specified in collection configuration file" },
  { E_BADVLDBINIT,	"Attempt to initialize VLDB library failed" },
  { E_BADVLDBFETCH,	"Attempt to fetch file system info from VLDB failed" },
  { E_BADVOLUME,	"File system volume is unattached or does not exist" },
  { E_AFSPROBLEM,	"Attempt to fetch file system info failed due to some problem with the file system" },


  { DE_CHOWNFAILED,	"Call to chown(2) failed" },
  { DE_CHMODFAILED,	"Call to chmod(2) failed" },
#ifdef USE_UTIME
  { DE_UTIMEFAILED,	"Call to utime(3) failed" },
#else /* USE_UTIME */
  { DE_UTIMESFAILED,	"Call to utimes(2) failed" },
#endif /* USE_UTIME */
};

/*  unused so far
static unsigned NDepotErrors = sizeof(DepotErrorList)/sizeof(DepotErrorList[0]);
*/

#endif /* _DEPOTERRORCODES_H */
/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/DepotErrorCodes.h,v $ */
