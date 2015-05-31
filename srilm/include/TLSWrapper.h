/*
 * TLSWrapper.h --
 *    A template that wraps a thread-local storage variable. If NO_TLS is
 *    defined then the macros in this file will simply create static 
 *    variables, thereby producing a single-threaded build. 
 *
 * Copyright (c) 2012, SRI International.  All Rights Reserved.
 */

#ifndef TLSWrapper_h
#define TLSWrapper_h

#include <stdlib.h>
#include <assert.h>

#include "tls.h"
#include "Boolean.h"

#ifndef NO_TLS
// Declare and define a POD TLS variable
#  define TLSW(type, name) TLSWrapper<type> name(1, false)
// Declare and define a non-POD TLS variable (call its constructor)
#  define TLSWC(type, name) TLSWrapper<type> name(1, true)
// Declare and define a TLS array
#  define TLSW_ARRAY(type, name, size) TLSWrapper<type> name(size)

// Declare a TLS variable of a given type
#  define TLSW_DECL(type, name) TLSWrapper<type> name
// Declare a TLS array of a given type
#  define TLSW_DECL_ARRAY(type, name, size) TLSWrapper<type> name

// Define a previously declared TLS variable
#  define TLSW_DEF(type, name) TLSWrapper<type> name = TLSWrapper<type>(1, false)
// Define a previously declared non-POD TLS variable
#  define TLSW_DEFC(type, name) TLSWrapper<type> name = TLSWrapper<type>(1, true)
// Define a previously declared array TLS variable
#  define TLSW_DEF_ARRAY(type, name, size) TLSWrapper<type> name = TLSWrapper<type>(size)

// Get a T reference that is specific to the current thread
#  define TLSW_GET(name) (name.get())
// Get a T pointer to the beginning of the array that belongs to current thread
#  define TLSW_GET_ARRAY(name) &name.get()

// Free the thread-local memory (but not what it points to, if anything)
#  define TLSW_FREE(name) name.release()

template<class T>
class TLSWrapper {
public:
    TLSWrapper(size_t numP = 1, Boolean constructP = false) {
        key = TLS_CREATE_KEY();
        num = numP;
        construct = constructP; 
    }
    
    ~TLSWrapper() {
        TLS_FREE_KEY(key);
    }

    T &get() {
        T* mem = (T*)TLS_GET(key);
        if (mem == 0) {
            // Since we're imitating static memory, zero-init
            if (construct)
                mem = new T();
            else
                mem = (T*)calloc(num, sizeof(T));
	    assert(mem != 0);
            TLS_SET(key, mem);
        }
        return *mem;
    }
   
    void release() {
        T* mem = (T*)TLS_GET(key);

        if (mem != 0) {
            if (construct)
                delete mem;
            else
                free(mem);

            TLS_SET(key, 0);
        }
    }
private:
   size_t num;
   Boolean construct;
   TLS_KEY key; 
};

#else
// Just create static variables for single-threaded builds
#  define TLSW(type, name) type name
#  define TLSWC(type, name) type name
#  define TLSW_ARRAY(type, name, size) type name[size]

#  define TLSW_DECL(type, name) type name
#  define TLSW_DECL_ARRAY(type, name, size) type name[size] 

#  define TLSW_DEF(type, name) type name;
#  define TLSW_DEFC(type, name) type name;
#  define TLSW_DEF_ARRAY(type, name, size) type name[];

#  define TLSW_GET(name) name
#  define TLSW_GET_ARRAY(name) name

#  define TLSW_FREE(name)
#endif

#endif /* TLSWrapper_h */

