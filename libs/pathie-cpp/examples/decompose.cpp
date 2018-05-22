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

/**
 * decompose -- Print decomposed path to standard output.
 *
 * Compile with:
 *   g++ -std=c++98 -I/path/to/pathie/include -L/path/to/pathie/lib decompose.cpp -static -lpathie -o decompose
 */

#include <iostream>
#include <pathie/path.hpp>

int main(int argc, char* argv[])
{
  if (argc == 1) {
    std::cout << "USAGE: " << std::endl << "decompose PATH" << std::endl;
    return 0;
  }

  Pathie::Path path(argv[1]);

  std::cout << "Target path: " << path << std::endl;
  std::cout << "Components: " << path.component_count() << std::endl;
  std::cout << "Parent: " << path.parent() << std::endl;
  std::cout << "Root: " << path.root() << std::endl;
  std::cout << "Basename: " << path.basename() << std::endl;
  std::cout << "Dirname: " << path.dirname() << std::endl;
  std::cout << "Extension: " << path.extension() << std::endl;
  std::cout << "Absolute path: " << path.absolute() << std::endl;
}
