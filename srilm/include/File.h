/*
 * File.h
 *	File I/O utilities for LM
 *
 * Copyright (c) 1995-2011 SRI International, 2012-2013 Microsoft Corp.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/CVS/srilm/misc/src/File.h,v 1.25 2013-03-21 06:31:23 stolcke Exp $
 *
 */

#ifndef _File_h_
#define _File_h_

#ifdef PRE_ISO_CXX
# include <iostream.h>
#else
# include <iostream>
using namespace std;
#endif

#include <stdio.h>

#include "zio.h"
#include "zlib.h"

#include "Boolean.h"

/*
 * Tell clients that we can handle .gz files regardless of zio working
 */
#undef GZIP_SUFFIX
#define GZIP_SUFFIX      ".gz"

const unsigned int maxWordsPerLine = 50000;

extern const char *wordSeparators;

/*
 * A File object is a wrapper around a stdio FILE pointer.  If presently
 * provides two kinds of convenience.
 *
 * - constructors and destructors manage opening and closing of the stream.
 *   The stream is checked for errors on closing, and the default behavior
 *   is to exit() with an error message if a problem was found.
 * - the getline() method strips comments and keeps track of input line
 *   numbers for error reporting.
 * 
 * File object can be cast to (FILE *) to perform most of the standard
 * stdio operations in a seamless way.
 *
 * The File object can also read/write to a std::string, for file
 * access "to memory".  
 *
 * To read from an existing string, allocate the File object using:
 * File(char *, size_t) or File(std::string&) and then call any File()
 * accessor function.  For reading, you can also allocate the File
 * object using File(NULL, exitOnError) and then reopen it using
 * File.reopen(char *, size_t) or File.reopen(std::string&).
 *
 * To write to a string, allocate the File object using: File("", 0,
 * exitOnError, reserved_length).  Alternatively, use File(NULL,
 * exitOnError) followed by File.reopen("", 0, reserved_length).
 *
 * NOTE: String I/O does not yet support binary data (unless initialized from std::string?).
 * NOTE: For backwards compatibility, File object preferentially uses FILE * object if it exists.
 */
class File
{
public:
    File(const char *name, const char *mode, int exitOnError = 1);
    File(FILE *fp = 0, int exitOnError = 1);
    // Initialize strFile with contents of string.  strFile will be
    // resized to "reserved_length" if this value is bigger than the
    // string size.
    File(const char *fileStr, size_t fileStrLen, int exitOnError = 1, int reserved_length = 0);
    File(std::string& fileStr, int exitOnError = 1, int reserved_length = 0);
    ~File();

    char *getline();
    void ungetline();
    int close();
    Boolean reopen(const char *name, const char *mode);
    Boolean reopen(const char *mode);		// switch to binary I/O
    // [close() and] reopen File and initialize strFile with contents of string
    Boolean reopen(const char *fileStr, size_t fileStrLen, int reserved_length = 0);
    Boolean reopen(std::string& fileStr, int reserved_length = 0);
    Boolean error();

    ostream &position(ostream &stream = cerr);
    ostream &offset(ostream &stream = cerr);

    const char *name;
    unsigned int lineno;
    Boolean exitOnError;
    Boolean skipComments;

    // Provide "stdio" equivalent functions for the case where the
    // File class is wrapping a string instead of a FILE, since
    // casting File to (FILE *) won't work in this case.  The
    // functions should perform the same as their namesakes, but will
    // not set errno.
    char *fgets(char *str, int n);
    char *fgetsUTF8(char *str, int n);	// also converts to UTF8
    int fgetc();
    int fputc(int c);
    int fputs(const char *str);
    // uses internal 4KB buffer
    int fprintf(const char *format, ...);
    size_t fread(void *data, size_t size, size_t n);
    size_t fwrite(const void *data, size_t size, size_t n);
    long long ftell();
    int fseek(long long offset, int origin);

    // get string contents from File() object, provided we are doing string I/O
    const char *c_str();
    const char *data();
    size_t length();

private:
    
    FILE *fp;
    gzFile gzf;			// when reading/writing via zlib
    
    char *buffer;
    unsigned bufLen;
    Boolean reuseBuffer;
    Boolean atFirstLine;	// we haven't read the first line yet
    enum { ASCII, UTF8, UTF16LE, UTF16BE } encoding;	// char encoding scheme
    void *iconvID;		

    // read/write from/to string instead of file
    std::string strFile;
    int strFileLen;
    int strFilePos;
    int strFileActive;
};

#endif /* _File_h_ */

