/*
 * tserror.h --
 *     Provide thread-safe strerror calls
 *
 * Copyright (c) 2012, SRI International.  All Rights Reserved.
 */

#ifndef tserror_h
#define tserror_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NO_TLS
char *srilm_ts_strerror(int errnum);
#else
# define srilm_ts_strerror strerror
#endif

void srilm_tserror_freeThread();

#ifdef __cplusplus
}
#endif

#endif /* tserror_h */

