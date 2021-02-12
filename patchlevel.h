static char DepotPatchlevel[] = "Depot $Revision: 4.8 $ $Date: 1992/08/14 18:58:20 $";
#define PATCHLEVEL 19

/* This log summarizes the major changes in depot as the versions go by
 *
 * $Log: patchlevel.h,v $
 * Revision 4.8  1992/08/14  18:58:20  ww0r
 * - clarified custom.depot man page
 * - fixed formatting errors in custom.depot man page
 * - changed errno reporting from the numeric values to the text string
 * - changed patchlevel.h to also provide a numeric PATCHLEVEL constant
 *
 * Revision 4.7  1992/07/30  22:32:01  sohan
 * Made mapping of virgin directory trees which are non-virgin only at the top-level more efficient
 * Fixed bugs with deleteunreferenced set.
 * Fixed bug with Override() not checking for *.override
 *
 * Revision 4.6  1992/07/11  00:16:50  ww0r
 * - moved Makefile.dist to Makefile.build and created a real Makefile.dist
 * - depot complain when invalid paths are specified in the custom.depot
 * - fixed bug with USE_UTIME
 *
 * Revision 4.5  1992/06/24  18:37:23  ww0r
 * fixed problem with memory allocation
 * minor performance enhancements
 *
 * Revision 4.4  1992/06/23  17:22:27  ww0r
 * minor code cleanups
 * allow ~afsmountpoint to exist in depot.conf's even
 * 	if USE_FSINFO isn't defined
 *
 * Revision 4.3  1992/06/19  20:40:57  ww0r
 * General Saber-C/lint type cleaning
 * removed all log messages from code
 * stopped exporting header foo in .h files
 *
 * Revision 4.2  1992/06/19  02:42:19  ww0r
 * globally changed ecalloc's to be emalloc's. This should be a minor
 * performance enhancement as malloc won't bother clearing blocks of
 * memory that will be immediately overwritten by a strcpy.
 *
 */
