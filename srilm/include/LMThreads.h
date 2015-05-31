/*
 * LMThreads.h
 * 
 * Provide mechanisms for freeing thread-specific resources when a thread
 * terminates long before the process.
 *
 * Copyright (c) 2012, SRI International.  All Rights Reserved.
 */

#ifndef LMThreads_h
#define LMThreads_h

#ifndef NO_TLS
class LMThreads {
public:
  static void freeThread();
};
#endif /* NO_TLS */

#endif /* LMThreads_h */

