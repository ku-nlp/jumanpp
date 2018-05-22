/* -*- coding: utf-8 -*-
 * This file is part of Pathie.
 *
 * Copyright © 2015, 2017 Marvin Gülker
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <cstring>
#include <locale>
#include "../include/pathie.hpp"
#include "testhelpers.hpp"

using namespace Pathie;

void test_transcode()
{
  std::string utf8str     = "\x42\xc3\xa4\x72\x20\x4d\xc3\xb6\x68\x72\x65\x20\x53\x6f\xc3\x9f\x65"; // UTF-8: Bär Möhre Soße
  std::string iso88591str = "\x42\xe4\x72\x20\x4d\xf6\x68\x72\x65\x20\x53\x6f\xdf\x65"; // ISO-8859-1: Bär Möhre Soße

  std::string result1 = convert_encodings("ISO-8859-1", "UTF-8", iso88591str);
  EQUAL(utf8str, result1);

  std::string result2 = convert_encodings("UTF-8", "ISO-8859-1", utf8str);
  EQUAL(iso88591str, result2);

  // No way to specify what iconv() shall do with characters that are
  // not available in the target encoding. POSIX.1-2008 just says the
  // iconv() function "shall perform an implementation-defined
  // conversion on this character." So, what to test here?

  // const char* unicodestr = "\xe2\x9c\x93"; // UTF-8: ✓
}

int main(int argc, char* argv[])
{
#ifndef _WIN32
  std::locale::global(std::locale(""));
#endif
  test_transcode();
}
