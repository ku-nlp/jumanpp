/*
 * Boolean Type
 *
 * Copyright (c) 1995,2006 SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/CVS/srilm/misc/src/Boolean.h,v 1.5 2006/01/09 17:39:03 stolcke Exp $
 *
 */

#ifndef _BOOLEAN_H_
#define _BOOLEAN_H_

#if defined(__GNUG__) || defined(__INTEL_COMPILER) || defined(__SUNPRO_CC) || defined (_MSC_VER)
typedef bool Boolean;

#else /* ! __GNUG__ && !__INTEL_COMPILER && !__SUNPRO_CC && !_MSC_VER */

typedef int Boolean;
const Boolean false = 0;
const Boolean true = 1;
#endif /* __GNUG __ || __INTEL_COMPILER || __SUNPRO_CC || _MSC_VER */

#endif /* _BOOLEAN_H_ */
