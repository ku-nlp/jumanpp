/*
    File:   mkdir.h
    Author: Andreas Stolcke
    Date:   Wed Feb 15 15:19:44 PST 1995
   
    Description: Portability for the mkdir function

    Copyright (c) 2006, SRI International.  All Rights Reserved.

    RCS ID: $Id: mkdir.h,v 1.2 2006/10/17 18:53:33 stolcke Exp $
*/

/*
 *  $Log: mkdir.h,v $
 *  Revision 1.2  2006/10/17 18:53:33  stolcke
 *  win32 portability
 *
 *  Revision 1.1  2006/01/09 19:14:04  stolcke
 *  Initial revision
 *
 */

#ifndef _MKDIR_H
#define _MKDIR_H

#if defined(_MSC_VER) || defined(WIN32)
# include <direct.h>
# define MKDIR(d)	_mkdir(d)
#else
# include <sys/stat.h>
# include <sys/types.h>
# ifdef S_IRWXO
#  define MKDIR(d)	mkdir(d, S_IRWXU|S_IRWXG|S_IRWXO)
# else
#  define MKDIR(d)	mkdir(d)
# endif
#endif /* _MSC_VER */

#endif /* _MKDIR_H */

