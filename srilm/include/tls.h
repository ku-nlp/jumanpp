/*
 * tls.h --
 *     Abstracts pthread and Windows thread-local storage mechanisms
 *
 * Copyright (c) 2012, SRI International.  All Rights Reserved.
 */

#ifndef tls_h
#define tls_h

#ifndef NO_TLS
#  if defined(_MSC_VER) || defined(WIN32)
#    include <windows.h>
#    define TLS_KEY DWORD 
#    define TLS_CREATE_KEY TlsAlloc 
#    define TLS_GET(key) TlsGetValue(key)
#    define TLS_SET(key, value) TlsSetValue(key, value)
#    define TLS_FREE_KEY(key) TlsFree(key)
#  else
#    include <pthread.h>
#    define TLS_KEY pthread_key_t
#    define TLS_CREATE_KEY srilm_tls_get_key
#    define TLS_GET(key) pthread_getspecific(key)
#    define TLS_SET(key, value) pthread_setspecific(key, value)
#    define TLS_FREE_KEY(key) pthread_key_delete(key) 
     TLS_KEY srilm_tls_get_key();
#  endif /* _MSC_VER */
#endif /* USE_TLS */

#endif /* tls_h */

