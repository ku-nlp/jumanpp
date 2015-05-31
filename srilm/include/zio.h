/*
    File:   zio.h
    Author: Andreas Stolcke
    Date:   Wed Feb 15 15:19:44 PST 1995
   
    Description:

    Copyright (c) 1994-2007, SRI International.  All Rights Reserved.

    RCS ID: $Id: zio.h,v 1.14 2009/08/22 22:41:19 stolcke Exp $
*/

/*
 *  $Log: zio.h,v $
 *  Revision 1.14  2009/08/22 22:41:19  stolcke
 *  support for xz compressed files
 *
 *  Revision 1.13  2007/11/11 16:06:53  stolcke
 *  7zip compression support
 *
 *  Revision 1.12  2006/08/04 23:59:09  stolcke
 *  MSVC portability
 *
 *  Revision 1.11  2006/03/28 01:15:10  stolcke
 *  include sys/signal.h to check for SIGPIPE
 *
 *  Revision 1.10  2006/03/06 05:46:43  stolcke
 *  define NO_ZIO in zio.h instead of zio.c
 *
 *  Revision 1.9  2006/03/01 00:45:45  stolcke
 *  allow disabling of zio for windows environment (NO_ZIO)
 *
 *  Revision 1.8  2005/12/16 23:30:09  stolcke
 *  added support for bzip2-compressed files
 *
 *  Revision 1.7  2003/02/21 20:18:53  stolcke
 *  avoid conflict if zopen is already defined in library
 *
 *  Revision 1.6  1999/10/13 09:07:13  stolcke
 *  make filename checking functions public
 *
 *  Revision 1.5  1995/06/22 19:58:26  stolcke
 *  ansi-fied
 *
 *  Revision 1.4  1995/06/12 22:56:37  tmk
 *  Added ifdef around the redefinitions of fopen() and fclose().
 *
 */

/*******************************************************************
   Copyright 1994 SRI International.  All rights reserved.
   This is an unpublished work of SRI International and is not to be
   used or disclosed except as provided in a license agreement or
   nondisclosure agreement with SRI International.
 ********************************************************************/


#ifndef _ZIO_H
#define _ZIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Include declarations files. */

#include <stdio.h>
#include <signal.h>		// to check for SIGPIPE

/* Avoid conflict with library function */
#ifdef HAVE_ZOPEN
#define zopen my_zopen
#endif

/* Constants */
#if !defined(SIGPIPE)
#define NO_ZIO
#endif

#ifdef NO_ZIO
# define COMPRESS_SUFFIX  ""
# define GZIP_SUFFIX	  ""
# define OLD_GZIP_SUFFIX  ""
# define BZIP2_SUFFIX	  ""
# define SEVENZIP_SUFFIX  ""
# define XZ_SUFFIX	  ""
#else
# define COMPRESS_SUFFIX  ".Z"
# define GZIP_SUFFIX	  ".gz"
# define OLD_GZIP_SUFFIX  ".z"
# define BZIP2_SUFFIX	  ".bz2"
# define SEVENZIP_SUFFIX  ".7z"
# define XZ_SUFFIX	  ".xz"
#endif /* NO_ZIO */

/* Define function prototypes. */

int	stdio_filename_p (const char *name);
int	compressed_filename_p (const char *name);
int 	gzipped_filename_p (const char *name);
int 	bzipped_filename_p (const char *name);
int 	sevenzipped_filename_p (const char *name);
int 	xz_filename_p (const char *name);

FILE *	zopen (const char *name, const char *mode);
int	zclose (FILE *stream);

/* Users of this header implicitly always use zopen/zclose in stdio */

#ifdef ZIO_HACK
#define fopen(name,mode)	zopen(name,mode)
#define fclose(stream)		zclose(stream)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ZIO_H */

