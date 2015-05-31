/*
 * MStringTokUtil.h --
 *      Platform-independent version of strtok_r.
 *
 * @author SRI International
 * @file MStringTokUtil.h \brief Utility for portable string tokenization.
 *
 * Copyright (C) 2011 SRI International.  Unpublished, All Rights Reserved.
 *
 * $Id: MStringTokUtil.h,v 1.1 2011/04/01 17:47:18 victor Exp $
 */

#ifndef MStringTokUtil_h
#define MStringTokUtil_h

/**
 * Platform-independent version of strtok_r.
 */
class MStringTokUtil {
  public:
    /**
     * Get next token from string based on character separators.
     *
     * @param s1 For the first call, this is a pointer to a string
     * from which to extract tokens.  This string will be updated
     * with 0 characters.  On subsequent calls, this parameter
     * should be NULL.
     * @param s2 Null-terminated set of delimiter characters.
     * This may updated on subsequent calls.
     * @param lasts This is an address to a pointer used for
     * storing state between calls.  The value will be set
     * on the first call and read/updated on successive calls.
     * @return pointer to NULL-terminated next token in s1
     * or NULL when no tokens remain.
     */
    static char* strtok_r(char* s1, const char* s2, char** lasts);

  private:
    // Static; no constructor
    MStringTokUtil();
};

#endif // MStringTokUtil_h
