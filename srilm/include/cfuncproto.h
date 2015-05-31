/*
 * cfuncproto.h --
 *
 *	Declarations of a macro supporting Ansi-C function prototypes in
 *	Sprite. This macro allow function prototypes to be defined 
 *	such that the code works on both standard and K&R C.
 *
 * Copyright 1990 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 * $Header: /home/srilm/CVS/srilm/misc/src/cfuncproto.h,v 1.9 2011/04/06 03:23:00 stolcke Exp $ SPRITE (Berkeley)
 */

#ifndef _CFUNCPROTO
#define _CFUNCPROTO

/*
 * Definition of the _ARGS_ macro.  The _ARGS_ macro such be used to 
 * enclose the argument list of a function prototype.  For example, the
 * function:
 * extern int main(argc, argv)
 *	int args;
 * 	char **argv;
 *
 * Would have a prototype of:
 *
 * extern int main _ARGS_((int argc, char **argv))
 *
 */

#ifndef _ASM

#if defined(__STDC__) || defined(_MSC_VER)
#define _HAS_PROTOTYPES
#define _HAS_VOIDPTR
#define _HAS_CONST
#endif

#if defined(__cplusplus)
#define _EXTERN         extern "C"
#define _NULLARGS	(void) 
#define _HAS_PROTOTYPES
#define _HAS_VOIDPTR
#define _HAS_CONST
#else 
#define _EXTERN         extern
#define _NULLARGS	() 
#endif

#if defined(_HAS_PROTOTYPES) && !defined(lint)
#define	_ARGS_(x)	x
#else
#define	_ARGS_(x)	()
#endif

#ifndef _CONST
#ifdef _HAS_CONST
#define _CONST          const
#else
#define _CONST
#endif
#endif

#ifdef _HAS_VOIDPTR
typedef void *_VoidPtr;
#else
typedef char *_VoidPtr;
#endif

#endif /* _ASM */
#endif /* _CFUNCPROTO */

