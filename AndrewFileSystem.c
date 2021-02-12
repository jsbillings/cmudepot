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

static char rcs_id[]="$Header: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/AndrewFileSystem.c,v 4.5 1992/06/19 20:15:56 ww0r Exp $";

/*
 * Author: Sohan C. Ramakrishna Pillai
 */

#include <stdio.h>
#include <sys/param.h>
#include <afs/param.h>
#include <afs/stds.h>
#include <rx/xdr.h>
#include <sys/ioctl.h>
#include <afs/cellconfig.h>
#include <afs/vice.h>
#include <afs/venus.h>
#include <afs/vlserver.h>
#include <afs/afsint.h>
#include <afs/volser.h>
#include <afs/volint.h>
#include <afs/vldbint.h>
#include <ubik.h>

#include "globals.h"
#include "depot.h"
#include "DepotDBStruct.h"
#include "DepotDB.h"
#include "CollectionStruct.h"
#include "Collection.h"
#include "AndrewFileSystem.h"

#define MAXSIZE 2*BUFSIZ

static volintInfo *AFS_vol_GetInfobyID();
static int GetServerAndPart();

static struct ubik_client *cstruct;
static struct rx_securityClass *sc;
static int securityindex = 0;
#define UV_BIND(s, p) rx_NewConnection((s), htons(p), VOLSERVICE_ID,sc,securityindex)


static int
GetServerAndPart(entry, voltype, server, part)
     struct vldbentry entry;
     long voltype, *server, *part;
{
  int i;

  dbgprint(F_TRACE, (stderr, "GetServerAndPart\n"));
  *server = -1;
  *part = -1;
  if ((voltype == RWVOL) || (voltype  == BACKVOL)) {
    for (i = 0; i < entry.nServers ; i++){
      if (entry.serverFlags[i] & ITSRWVOL){
	*server = entry.serverNumber[i];
	*part = entry.serverPartition[i];
	dbgprint(F_TRACE, (stderr, "GetServerAndPart done\n"));
        return(1);
      }
    }
  }
  if (voltype == ROVOL) {
    for (i = 0; i < entry.nServers ; i++) {
      if ((entry.serverFlags[i] & ITSROVOL)) {
	*server = entry.serverNumber[i];
	*part = entry.serverPartition[i];
	dbgprint(F_TRACE, (stderr, "GetServerAndPart done\n"));
	return(1);
      }
    }
  }
  dbgprint(F_TRACE, (stderr, "GetServerAndPart done\n"));
  return(-1);
}

/* return of NULL  means there is an error somewhere
 * returns volinfoInfo, this needs to be freed when done
 */
static volintInfo *
AFS_vol_GetInfobyID(volid)
     long volid;
{
  static volintInfo *volinfo;
  struct vldbentry entry;
  long code;
  int voltype;
  long aserver,apart;
  extern int VL_GetEntryByID();
  char volbusy;
  struct rx_connection *aconn;
  
  dbgprint(F_TRACE, (stderr, "AFS_vol_GetinfobyID\n"));
  if (code = ubik_Call(VL_GetEntryByID, cstruct, 0, volid,-1, &entry)) {
    FatalError(E_AFSPROBLEM, (stderr,
	    "Could not get the VLDB entry for %d\n",volid));
  }
  MapHostToNetwork(&entry);
  if (entry.volumeId[RWVOL] == volid) {
    voltype = RWVOL;
  } else if (entry.volumeId[ROVOL] == volid) {
    voltype = ROVOL;
  } else  if (entry.volumeId[BACKVOL] == volid) {
    voltype = BACKVOL;
  }
    
  GetServerAndPart(entry, voltype, &aserver,&apart);
  if ((voltype == RWVOL) &&(aserver == -1 || apart == -1)) {
    FatalError(E_AFSPROBLEM, 
	       (stderr,"Could not find volume (%d) on any servers\n",
		volid));
  }
  volbusy = 1;
  aconn = UV_BIND(aserver,AFSCONF_VOLUMEPORT);

  while (volbusy) {
    volEntries volumeInfo;

    volumeInfo.volEntries_val = (volintInfo *)0;/*this hints the stub to allocated */
    volumeInfo.volEntries_len = 0;
    
    if (AFSVolListOneVolume(aconn, apart, volid, &volumeInfo)) {
      FatalError(E_AFSPROBLEM, (stderr,"Unable to get info on %d, code = %d\n", 
				volid, code));
    }
    volinfo = volumeInfo.volEntries_val;
    if (volbusy = (volinfo->status == VBUSY)) {
      fprintf(stderr,"** Waiting for busy volume %d **\n", volinfo->volid);
      sleep(5);
    }
  }
  if (aconn)
    rx_DestroyConnection(aconn);

  dbgprint(F_TRACE, (stderr, "AFS_vol_GetinfobyID done\n"));
  return(volinfo);
}

int
AFS_vol_init()
{
  struct afsconf_cell info;
  struct afsconf_dir *tdir;
  static struct rx_connection *serverconns[VLDB_MAXSERVERS];
  long i;

  dbgprint(F_TRACE, (stderr, "AFS_vol_init\n"));
  if (rx_Init(0))
    return(-1);
/*  rx_SetRxDeadTime(50); */
  if ((tdir = afsconf_Open(AFSCONF_CLIENTNAME)) == (struct afsconf_dir *) NULL) {
    return(-2);
  }
  if (afsconf_GetCellInfo(tdir, NULL, AFSCONF_VLDBSERVICE, &info)) {
    return(-3);
  }
  if (info.numServers > VLDB_MAXSERVERS) { 
    return(-4);
  }
  sc = (struct rx_securityClass *)rxnull_NewClientSecurityObject();
  for (i = 0;i<info.numServers;i++)
    serverconns[i] = rx_NewConnection(info.hostAddr[i].sin_addr.s_addr,
				      info.hostAddr[i].sin_port,
				      USER_SERVICE_ID, 
				      sc, securityindex);
  if (ubik_ClientInit(serverconns, &cstruct)) {
    return(-5);
  }

  dbgprint(F_TRACE, (stderr, "AFS_vol_init done\n"));
  return(0);
}

void
AFS_vol_done()
{
  dbgprint(F_TRACE, (stderr, "AFS_vol_done\n"));
  rx_Finalize();
  dbgprint(F_TRACE, (stderr, "AFS_vol_done done\n"));
}

void AFS_GetVolumeInfo(fsinfop)
     COLLECTIONFSINFO *fsinfop;
{
  char buf[MAXSIZE];
  register long code;
  struct ViceIoctl blob;
  struct VolumeStatus *status;
  volintInfo *volinfo;
  char *volname;

  dbgprint(F_TRACE, (stderr, "AFS_GetVolumeInfo\n"));
  blob.out_size = MAXSIZE;
  blob.in_size = 0;
  blob.out = buf;
  code = pioctl(fsinfop->path, VIOCGETVOLSTAT, &blob, 1);
  if (code)
    {
      /* Not an AFS thingie */
      fsinfop->fs_id = -1; fsinfop->fs_modtime = 0;
      dbgprint(F_AFSINFO, (stderr, "No AFS volume for path %s\n", fsinfop->path));
    }
  else
    {
      status = (VolumeStatus *)buf;
      volname = (char *)status + sizeof(*status);
      fsinfop->fs_id = status->Vid;
      dbgprint(F_AFSINFO, (stderr, "Located AFS volume %s id %d for path %s\n", volname, fsinfop->fs_id, fsinfop->path));
      volinfo = AFS_vol_GetInfobyID(fsinfop->fs_id);
      fsinfop->fs_modtime = volinfo->updateDate;
      free(volinfo);
      dbgprint(F_AFSINFO, (stderr, "Path %s has volume %s with id %d and modtime %ld = %s\n", fsinfop->path, volname, fsinfop->fs_id, fsinfop->fs_modtime, ctime(&fsinfop->fs_modtime)));
    }

  dbgprint(F_TRACE, (stderr, "AFS_GetVolumeInfo done\n"));
  return;
}

void AFS_GetMountPointInfo(fsinfop)
     COLLECTIONFSINFO *fsinfop;
{
  char buf[MAXSIZE];
  char parent_dir[MAXPATHLEN];
  register char *last_component, *last_slash;
  register long code;
  struct ViceIoctl blob;

  dbgprint(F_TRACE, (stderr, "AFS_GetMountPointInfo\n"));
  /*
   * split up the path in fsinfop->path
   * into the last component and parent directory
   */
  last_slash = (char *) rindex(fsinfop->path, '/');
  if (last_slash) /* we have a / in the path */
    {
      /*
       * Designate everything after the last slash as the last component
       * and everything before it as the parent directory;
       */
      (void)strncpy(parent_dir, fsinfop->path, last_slash - fsinfop->path);
      parent_dir[last_slash - fsinfop->path] = '\0';
      last_component = last_slash + 1;
    }
  else /* no / in path */
    {
      /* Designate . as the parent directory */
      (void)strcpy(parent_dir, ".");
      last_component = fsinfop->path;
    }

  if (strcmp(last_component, ".") == 0 || strcmp(last_component, "..") == 0)
    {
      fsinfop->fs_id = -1; fsinfop->fs_modtime = 0;
      dbgprint(F_AFSINFO, (stderr, "No AFS mountpoint searched for path %s because last component is %s\n", fsinfop->path, last_component));
    }
  else
    {
      blob.out_size = MAXSIZE;
      blob.out = buf;
      blob.in = last_component;
      blob.in_size = strlen(last_component)+1;
      code = pioctl(parent_dir, VIOC_AFS_STAT_MT_PT, &blob, 1);
      if (code)
	{
	  /* Not an AFS thingie */
	  fsinfop->fs_id = -1; fsinfop->fs_modtime = 0;
	  dbgprint(F_AFSINFO, (stderr, "No AFS mountpoint at path %s\n", fsinfop->path));
	}
      else
	{
	  /*
	   * Now that we know we have a mountpoint,
	   * reuse the AFS_GetVolumeInfo code
	   * rather than do the uglier but more efficient
	   *and probably less obsolesence-resistant removal
	   * of the # in the front of buf
	   */
	  dbgprint(F_AFSINFO, (stderr, "Located AFS mountpoint %s for path %s\n", buf, fsinfop->path));
	  AFS_GetVolumeInfo(fsinfop);
	}
    }
  dbgprint(F_TRACE, (stderr, "AFS_GetMountPointInfo done\n"));
  return;

}

/* $Source: /afs/andrew.cmu.edu/system/src/local/depot/019/RCS/AndrewFileSystem.c,v $ */
