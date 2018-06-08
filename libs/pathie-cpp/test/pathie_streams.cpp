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
#include "../include/pathie_ifstream.hpp"
#include "../include/pathie_ofstream.hpp"
#include "testhelpers.hpp"

#if defined(_PATHIE_UNIX)
#include <unistd.h>
#include <limits.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#error Unsupported system.
#endif

using namespace Pathie;

void test_ofstream()
{
  Path p1("foo");

  p1.mkdir();
  Path p2("foo/bar.txt");

  Pathie::ofstream file(p2, Pathie::ofstream::out);
  IS_TRUE(file.is_open());
  IS_TRUE(file.good());

  file << "Test stuff" << std::endl;
  file.close();

  IS_TRUE(p2.exists());

  char buf[256];
  memset(buf, '\0', 256);
  FILE* p_file = p2.fopen("r");
  fread(buf, 1, 256, p_file);
  fclose(p_file);

  EQUAL("Test stuff\n", buf);

  Path p3("foo/bär blaß.txt");
  Pathie::ofstream unicode_file(p3, Pathie::ofstream::out);
  IS_TRUE(unicode_file.is_open());
  IS_TRUE(unicode_file.good());

  unicode_file << "Bärenstark" << std::endl;
  unicode_file.close();

  IS_TRUE(p3.exists());

  memset(buf, '\0', 256);
  p_file = p3.fopen("r");
  fread(buf, 1, 256, p_file);
  fclose(p_file);

  EQUAL("Bärenstark\n", buf);

  Pathie::ofstream stream;
  IS_FALSE(stream.is_open());
  stream.open("foo/foobarbaz.txt");
  IS_TRUE(stream.is_open());
  stream.close();
  IS_FALSE(stream.is_open());
}

void test_ifstream()
{
  Path p1("testfile.txt");
  Path p2("tästfile.txt");

  Pathie::ifstream file(p1);
  IS_TRUE(file.is_open());
  IS_TRUE(file.good());

  char buf[256];
  memset(buf, '\0', 256);
  file.read(buf, 256);
  EQUAL("There is some testtext\nin this file.\n", buf);

  Pathie::ifstream unicode_file(p2);
  IS_TRUE(unicode_file.is_open());
  IS_TRUE(unicode_file.good());

  memset(buf, '\0', 256);
  unicode_file.read(buf, 256);
  EQUAL("Thäre is ßöme testtext\nin this file.\n", buf);

  Pathie::ifstream stream;
  IS_FALSE(stream.is_open());
  stream.open("testfile.txt");
  IS_TRUE(stream.is_open());
  stream.close();
  IS_FALSE(stream.is_open());
}

int main(int argc, char* argv[])
{
#ifndef _WIN32
  std::locale::global(std::locale(""));
#endif
  std::cout << "Cleaning." << std::endl;
  system("rake clean");

  test_ofstream();
  test_ifstream();
}
