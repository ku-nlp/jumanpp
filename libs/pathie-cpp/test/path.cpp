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
#include <algorithm>
#include <locale>
#include <map>
#include <fstream>
#include "../include/path.hpp"
#include "../include/errors.hpp"
#include "testhelpers.hpp"

#if defined(_PATHIE_UNIX)
#include <unistd.h>
#include <limits.h>
#include <sys/param.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

static std::map<std::string, std::string> s_testsettings;

using namespace Pathie;

void test_constructor()
{
  Path p1;
  Path p2("/test/foo");
  Path p3(p2);
  Path p4(p2.burst());

  EQUAL(".", p1.str());
  EQUAL("/test/foo", p2.str());
  EQUAL("/test/foo", p3.str());
  EQUAL("/test/foo", p4.str());
}

void test_sanitizer()
{
  Path p1("test/");
  Path p2("test//foo");
  Path p3("test/foo//");
  Path p4("/test/");
  Path p5("/");
  Path p6("C:\\\\Windows\\path");
  Path p7("C:/");

  EQUAL("test", p1.str());
  EQUAL("test/foo", p2.str());
  EQUAL("test/foo", p3.str());
  EQUAL("/test", p4.str());
  EQUAL("/", p5.str());
  EQUAL("C:/Windows/path", p6.str());

#if defined(_PATHIE_UNIX)
  EQUAL("C:", p7.str()); // Relative path with trailing / under UNIX
#elif defined(_WIN32)
  EQUAL("C:/", p7.str()); // Absolute path with drive root / under Win32
#endif
}

void test_dirname_basename()
{
  Path p1("/usr/lib");
  Path p2("/usr");
  Path p4("usr");
  Path p5("/");
  Path p6(".");
  Path p7("..");
  Path p8("usr/lib/foo.txt");

  EQUAL("/usr", p1.dirname());
  EQUAL("lib", p1.basename());
  EQUAL("/", p2.dirname());
  EQUAL("usr", p2.basename());
  EQUAL(".", p4.dirname());
  EQUAL("usr", p4.basename());
  EQUAL("/", p5.dirname());
  EQUAL(".", p6.dirname());
  EQUAL(".", p6.basename());
  EQUAL(".", p7.dirname());
  EQUAL("..", p7.basename());
  EQUAL("usr/lib", p8.dirname());
  EQUAL("foo.txt", p8.basename());
}

void test_relative_absolute()
{
  Path p1("/foo");
  Path p2("foo");
  Path p3("foo/bar");
  Path p4(".");
  Path p5("..");
  Path p6("/");
  Path p7("C:/Windows");
  Path p8("C:/");

  IS_TRUE(p1.is_absolute());
  IS_FALSE(p1.is_relative());
  IS_FALSE(p2.is_absolute());
  IS_TRUE(p2.is_relative());
  IS_FALSE(p3.is_absolute());
  IS_TRUE(p3.is_relative());
  IS_FALSE(p4.is_absolute());
  IS_TRUE(p4.is_relative());
  IS_FALSE(p5.is_absolute());
  IS_TRUE(p5.is_relative());
  IS_TRUE(p6.is_absolute());
  IS_FALSE(p6.is_relative());

  IS_TRUE(p6.is_root()); // / on Windows is root on current drive
  IS_FALSE(p7.is_root());

  EQUAL("/foo", p1.absolute());
  EQUAL("/", p6.absolute());

#if defined(_PATHIE_UNIX)
  IS_FALSE(p7.is_absolute());
  IS_FALSE(p8.is_root());
  IS_FALSE(p8.is_absolute());

  EQUAL(Path::pwd().join(p8).str(), p8.absolute());
#elif defined(_WIN32)
  IS_TRUE(p7.is_absolute());
  IS_TRUE(p8.is_root());
  IS_TRUE(p8.is_absolute());

  EQUAL("C:/", p8.absolute());
#endif

  Path p9 = p3.absolute();
  EQUAL(Path::pwd().join("foo").join("bar").str(), p9);

  Path p10 = p3.absolute(Path("/tmp"));
  EQUAL("/tmp/foo/bar", p10);

  Path p11("/tmp/foo/bar/baz");
  Path p12("/tmp/xxx/yyy");
  Path p13("/");

  EQUAL("../../foo/bar/baz", p11.relative(p12));
  EQUAL("../../../xxx/yyy", p12.relative(p11));
  EQUAL(".", p11.relative(p11));
  EQUAL("tmp/xxx/yyy", p12.relative(p13));
  EQUAL("../../..", p13.relative(p12));
}

void test_root()
{
  Path p1("foo/bar");
  Path p2("/foo/bar");
  Path p3("C:/foo/bar");

  EQUAL("/", p1.root());
  EQUAL("/", p2.root());

#if defined(_PATHIE_UNIX)
  EQUAL("/", p3.root());
#elif defined(_WIN32)
  EQUAL("C:/", p3.root());
#endif
}

void test_extension()
{
  Path p1("/foo/bar.txt");
  Path p2("/foo");
  Path p3("foo.");
  Path p4("..");
  Path p5(".txt");
  Path p6("test.rb.qq");
  Path p7("foo.bar.");
  Path p8("/foo/.txt");

  EQUAL(".txt", p1.extension());
  EQUAL("", p2.extension());
  EQUAL("", p3.extension());
  EQUAL("", p4.extension());
  EQUAL("", p5.extension());
  EQUAL(".qq", p6.extension());
  EQUAL("", p7.extension());
  EQUAL("", p8.extension());
}

void test_operators()
{
  Path p1("/foo");
  Path p2("bar");

  EQUAL("/foo/bar.txt", p1 / Path("bar.txt"));
  EQUAL("/foo/bar.txt", p1 / "bar.txt");
  EQUAL("bar/foo/test.txt", p2 / Path("foo") / "test.txt");

  Path p3("/foo/bar/baz");
  Path p4("foo/bar/baz");
  EQUAL("/", p3[0].str());
  EQUAL("foo", p3[1].str());
  EQUAL("bar", p3[2].str());
  EQUAL("baz", p3[3].str());
  EQUAL("foo", p4[0].str());
  EQUAL("bar", p4[1].str());
  EQUAL("baz", p4[2].str());

  assert_exception(new std::out_of_range("."), __FILE__, __LINE__, []()
  {
    Path p("/foo/bar");
    std::cout << p[15].str() << std::endl;
  });

  Path p5("/");

  EQUAL(1, p5.component_count());
  EQUAL(2, p1.component_count());
  EQUAL(1, p2.component_count());
  EQUAL(4, p3.component_count());
  EQUAL(3, p4.component_count());
}

void test_prune()
{
  Path p1("/foo/bar/../baz.txt");
  EQUAL("/foo/baz.txt", p1.prune());

  Path p2("/foo/bar/../bar/../baz.txt");
  EQUAL("/foo/baz.txt", p2.prune());

  Path p3("/foo/bar/./baz.txt");
  EQUAL("/foo/bar/baz.txt", p3.prune());

  Path p4("/foo/bar/./blubb/./baz.txt");
  EQUAL("/foo/bar/blubb/baz.txt", p4.prune());

  Path p5("/foo/bar/.././blubb/./baz.txt");
  EQUAL("/foo/blubb/baz.txt", p5.prune());

  Path p6("/./baz.txt");
  EQUAL("/baz.txt", p6.prune());

  Path p7("/../baz.txt");
  EQUAL("/baz.txt", p7.prune());

  Path p8("foo/../baz.txt");
  EQUAL("baz.txt", p8.prune());

  Path p9("/..");
  EQUAL("/", p9.prune());

  Path p10("/../../././..");
  EQUAL("/", p10.prune());

  Path p11("/");
  EQUAL("/", p11.prune());

  Path p12("/.");
  EQUAL("/", p12.prune());

  Path p13("./..");
  EQUAL(".", p13.prune());

  Path p14(".");
  EQUAL(".", p14.prune());

  Path p15("..");
  EQUAL("..", p15.prune());

  // The following are under UNIX relative paths!
#ifdef _WIN32
  Path p16("C:/foo/bar/../xxx");
  EQUAL("C:/foo/xxx", p16.prune());

  Path p17("C:/foo/./bar");
  EQUAL("C:/foo/bar", p17.prune());

  Path p18("C:/foo/../..");
  EQUAL("C:/", p18.prune());

  Path p19("C:/.");
  EQUAL("C:/", p19.prune());

  Path p20("C:/..");
  EQUAL("C:/", p20.prune());

  Path p21("C:/");
  EQUAL("C:/", p21.prune());
#endif

  Path p22("/..foo");
  EQUAL("/..foo", p22.prune());

  Path p23("/.foo");
  EQUAL("/.foo", p23.prune());

  Path p24("bar/../..foo");
  EQUAL("..foo", p24.prune());

  Path p25("bar/..foo");
  EQUAL("bar/..foo", p25.prune());

  Path p26("bar/.foo");
  EQUAL("bar/.foo", p26.prune());

  Path p27("..foo/..bar");
  EQUAL("..foo/..bar", p27.prune());

  Path p28("foo../bar");
  EQUAL("foo../bar", p28.prune());

  Path p29("/..foo/bar/../..baz../blubb./.foo/..foo/aa../.bar/xxx/./yyy");
  EQUAL("/..foo/..baz../blubb./.foo/..foo/aa../.bar/xxx/yyy", p29.prune());

  Path p30("foo../../xxx../foo");
  EQUAL("xxx../foo", p30.prune());

  // Special case. We can’t remove this ../ because we don’t know what
  // the parent directory is without accessing the filesystem.
  Path p31("../foo");
  EQUAL("../foo", p31.prune());

#ifdef _WIN32
  Path p32("C:/..foo");
  EQUAL("C:/..foo", p32.prune());

  Path p33("C:/foo/..foo/../foo");
  EQUAL("C:/foo/foo", p33.prune());

  Path p34("C:/.foo");
  EQUAL("C:/.foo", p34.prune());

  Path p35("C:/..foo/.foo/..../x");
  EQUAL("C:/..foo/.foo/..../x", p35.prune());
#endif

  Path p36("foo/...../x");
  EQUAL("foo/...../x", p36.prune());
}

void test_pwd()
{
#if defined(_PATHIE_UNIX)
  char cwd[PATH_MAX];
  getcwd(cwd, PATH_MAX);

  std::string pwd(cwd);
#elif defined(__WIN32)
  wchar_t cwd[MAX_PATH];
  GetCurrentDirectoryW(MAX_PATH, cwd);

  std::string pwd(utf16_to_utf8(cwd));
  size_t pos = 0;
  while ((pos = pwd.find("\\")) != std::string::npos) { // Assignment intended
    pwd.replace(pos, 1, "/");
  }
#endif

  EQUAL(pwd, Path::pwd());
}

void test_absolute()
{
  Path p1("foo");
  Path p2("foo/bar");
  Path p3("/foo/bar");
  Path p4("~/foo");
  Path p5("~foo/bar");

  EQUAL(Path::pwd().join("foo").str(), p1.absolute());
  EQUAL(Path::pwd().join("foo/bar").str(), p2.absolute());
  EQUAL("/foo/bar", p3.absolute());
  EQUAL(Path::pwd().join("~/foo").str(), p4.absolute());
  EQUAL(Path::pwd().join("~foo/bar").str(), p5.absolute());
}

void test_expand()
{
  std::string str("~");
#if defined(_PATHIE_UNIX)

  Path p1(str);
  Path p2(str + s_testsettings["username"]);
  Path p3("~/foo/bar");
  Path p4(str + s_testsettings["username"] + "/foo/bar");
  Path p8(str + s_testsettings["username"] + "/foo/../bar");

  EQUAL(s_testsettings["homedir"], p1.expand());
  EQUAL(str + s_testsettings["username"], p2.expand());
  EQUAL(s_testsettings["homedir"] + "/foo/bar", p3.expand());
  EQUAL(str + s_testsettings["username"] + "/foo/bar", p4.expand());
  EQUAL(str + s_testsettings["username"] + "/bar", p8.expand());

#elif defined(__WIN32)
  std::string str2("C:");
  Path p1(str);
  Path p2(str + s_testsettings["username"]);
  Path p3("~/foo/bar");
  Path p4(str + s_testsettings["username"] + "/foo/bar");

  EQUAL(s_testsettings["homedir"], p1.expand());
  EQUAL(str + s_testsettings["username"], p2.expand());
  EQUAL(s_testsettings["homedir"] + "/foo/bar", p3.expand());
  EQUAL(str + s_testsettings["username"] + "/foo/bar", p4.expand());
#endif

  Path p5("~§/error");
  Path p6("also/~/wrong");
  Path p7(std::string("also") + "/~" + s_testsettings["username"] + "/wrong");

  EQUAL("~§/error", p5.expand());
  EQUAL(Path::pwd().str() + "/" + "also/~/wrong", p6.expand());
  EQUAL(Path::pwd().str() + "/" + "also/~" + s_testsettings["username"] + "/wrong", p7.expand());
}

void test_stat()
{
  Path p1("testfile.txt");

#if defined(_PATHIE_UNIX)
  EQUAL(37, p1.size());
  struct stat* p_s = p1.stat();
#elif defined(_WIN32)
  EQUAL(39, p1.size()); // Windows newlines
  struct _stat* p_s = p1.stat();
#else
#error Unsupported system.
#endif

  EQUAL(37, p_s->st_size);
  free(p_s);

  assert_exception(new Pathie::ErrnoError(1), __FILE__, __LINE__, []()
  {
    Path p("doesnotexist");
    p.stat();
  });

  assert_exception(new Pathie::ErrnoError(1), __FILE__, __LINE__, []()
  {
    Path p("doesnotexist");
    p.size();
  });
}

void test_system_paths()
{
  Path p1 = Path::exe();
  Path pwd = Path::pwd();

  EQUAL(pwd.join("path.test").str(), p1.str());

  Path data_dir = Path::data_dir();
  Path config_dir = Path::config_dir();
  Path cache_dir = Path::cache_dir();
  Path runtime_dir = Path::runtime_dir();
  Path temp_dir = Path::temp_dir();

#if defined(_PATHIE_UNIX)
  EQUAL(s_testsettings["datadir"], data_dir);
  EQUAL(s_testsettings["configdir"], config_dir);
  EQUAL(s_testsettings["cachedir"], cache_dir);
  EQUAL(s_testsettings["tempdir"], temp_dir);

  // Not defining $XDG_RUNTIME_DIR is against the XDG spec,
  // but some broken systems may still not define it.
  // The XDG spec recommends printing a warning message in such
  // cases.
  if (getenv("XDG_RUNTIME_DIR"))
    EQUAL(getenv("XDG_RUNTIME_DIR"), runtime_dir);
  else {
    std::cerr << "This system violates the XDG specification by not defining $XDG_RUNTIME_DIR. Testing fallback instead." << std::endl;
    EQUAL(s_testsettings["tempdir"], runtime_dir);
  }

  Path tmpdir = Path::mktmpdir();
  IS_TRUE(tmpdir.exists());
  struct stat* p_stat = tmpdir.stat();
  IS_TRUE(S_ISDIR(p_stat->st_mode));
  IS_TRUE(p_stat->st_mode & S_IRWXU);
  IS_FALSE(p_stat->st_mode & S_IRGRP);
  IS_FALSE(p_stat->st_mode & S_IROTH);
  free(p_stat);
  tmpdir.rmtree();
  IS_FALSE(tmpdir.exists());

  EQUAL(s_testsettings["desktopdir"], Path::desktop_dir());
  EQUAL(s_testsettings["documentsdir"], Path::documents_dir());
  EQUAL(s_testsettings["downloaddir"], Path::download_dir());
  EQUAL(s_testsettings["templatesdir"], Path::templates_dir());
  EQUAL(s_testsettings["publicsharedir"], Path::publicshare_dir());
  EQUAL(s_testsettings["musicdir"], Path::music_dir());
  EQUAL(s_testsettings["picturesdir"], Path::pictures_dir());
  EQUAL(s_testsettings["videosdir"], Path::videos_dir());

  EQUAL("/var/lib"                      , Path::global_mutable_data_dir(Path::LOCALPATH_NORMAL));
  EQUAL("/var/local/lib"                , Path::global_mutable_data_dir(Path::LOCALPATH_LOCAL));
  EQUAL("/usr/share"                    , Path::global_immutable_data_dir(Path::LOCALPATH_NORMAL));
  EQUAL("/usr/local/share"              , Path::global_immutable_data_dir(Path::LOCALPATH_LOCAL));
  EQUAL("/etc"                          , Path::global_config_dir(Path::LOCALPATH_NORMAL));
  EQUAL("/usr/local/etc"                , Path::global_config_dir(Path::LOCALPATH_LOCAL));
  EQUAL("/var/cache"                    , Path::global_cache_dir(Path::LOCALPATH_NORMAL));
  EQUAL("/var/local/cache"              , Path::global_cache_dir(Path::LOCALPATH_LOCAL));
  EQUAL("/usr/share/applications"       , Path::global_appentries_dir(Path::LOCALPATH_NORMAL));
  EQUAL("/usr/local/share/applications" , Path::global_appentries_dir(Path::LOCALPATH_LOCAL));
  EQUAL("/opt"                          , Path::global_programs_dir());

  if (Path("/run").exists()) {
    EQUAL("/run", Path::global_runtime_dir(Path::LOCALPATH_NORMAL));
  }
  else {
    EQUAL("/var/run", Path::global_runtime_dir(Path::LOCALPATH_NORMAL));
  }
  EQUAL("/var/local/run", Path::global_runtime_dir(Path::LOCALPATH_LOCAL));

  // default is LOCALPATH_LOCAL
  EQUAL(Path::LOCALPATH_LOCAL, Path::get_global_dir_default());
  EQUAL("/usr/local/share", Path::global_immutable_data_dir());

  Path::set_global_dir_default(Path::LOCALPATH_LOCAL);
  EQUAL("/usr/local/share", Path::global_immutable_data_dir());

  Path::set_global_dir_default(Path::LOCALPATH_NORMAL);
  EQUAL("/usr/share", Path::global_immutable_data_dir());

  // Reset for later calls
  Path::set_global_dir_default(Path::LOCALPATH_NORMAL);
#elif defined(_WIN32)
  EQUAL(s_testsettings["datadir"], data_dir);
  EQUAL(s_testsettings["configdir"], config_dir);
  EQUAL(s_testsettings["cachedir"], cache_dir);
  EQUAL(s_testsettings["tempdir"], runtime_dir);
  EQUAL(s_testsettings["tempdir"], temp_dir);

  Path tmpdir = Path::mktmpdir();
  IS_TRUE(tmpdir.exists());
  struct _stat* p_stat = tmpdir.stat();
  IS_TRUE(S_ISDIR(p_stat->st_mode));
  free(p_stat);
  tmpdir.rmtree();
  IS_FALSE(tmpdir.exists());

  EQUAL(s_testsettings["desktopdir"], Path::desktop_dir());
  EQUAL(s_testsettings["documentsdir"], Path::documents_dir());
  SKIP(); // download_dir() on Windows requires KNOWNFOLDERID, which MinGW does not support yet
  //EQUAL(s_testsettings["downloadsdir"], Path::download_dir());
  EQUAL(s_testsettings["templatesdir"], Path::templates_dir());
  EQUAL(s_testsettings["publicsharedir"], Path::publicshare_dir());
  EQUAL(s_testsettings["musicdir"], Path::music_dir());
  EQUAL(s_testsettings["picturesdir"], Path::pictures_dir());
  EQUAL(s_testsettings["videosdir"], Path::videos_dir());

  EQUAL("C:/All Users/Application Data", Path::global_mutable_data_dir());
  EQUAL("C:/Windows/system32", Path::global_immutable_data_dir());
  EQUAL("C:/Windows/system32", Path::global_config_dir());
  EQUAL("C:/All Users/Application Data", Path::global_cache_dir());
  EQUAL("C:/Temp", Path::global_runtime_dir());
  EQUAL("C:/ProgramData/Microsoft/Windows/Start Menu", Path::global_appentries_dir());
  EQUAL("C:/Program Files", Path::global_programs_dir());

  // the `local' parameter is ignored on Windows
  EQUAL(Path::LOCALPATH_LOCAL, Path::get_global_dir_default());
  EQUAL("C:/Windows/system32", Path::global_immutable_data_dir());

  Path::set_global_dir_default(Path::LOCALPATH_LOCAL);
  EQUAL("C:/Windows/system32", Path::global_immutable_data_dir());

  Path::set_global_dir_default(Path::LOCALPATH_NORMAL);
  EQUAL("C:/Windows/system32", Path::global_immutable_data_dir());

  // Reset for later calls
  Path::set_global_dir_default(Path::LOCALPATH_NORMAL);
#endif
}

void test_splits()
{
  Path p1("/foo/bar/baz");
  Path p2("foo/bar/baz");
  Path p3("foo");

  std::vector<Path> v1 = p1.burst();
  std::vector<Path> v2 = p2.burst();
  std::vector<Path> v3 = p3.burst();
  std::vector<Path> v4 = p1.burst(true);
  std::vector<Path> v5 = p2.burst(true);
  std::vector<Path> v6 = p3.burst(true);

  EQUAL("/", v1[0]);
  EQUAL("foo", v1[1]);
  EQUAL("bar", v1[2]);
  EQUAL("baz", v1[3]);

  EQUAL("foo", v2[0]);
  EQUAL("bar", v2[1]);
  EQUAL("baz", v2[2]);

  EQUAL("foo", v3[0]);

  EQUAL("/", v4[0]);
  EQUAL("/foo", v4[1]);
  EQUAL("/foo/bar", v4[2]);
  EQUAL("/foo/bar/baz", v4[3]);

  EQUAL("foo", v5[0]);
  EQUAL("foo/bar", v5[1]);
  EQUAL("foo/bar/baz", v5[2]);

  EQUAL("foo", v6[0]);
}

void test_exists()
{
  Path p1("testfile.txt");
  Path p2("doesnotexist");

  IS_TRUE(p1.exists());
  IS_FALSE(p2.exists());
}

void test_stats()
{
  Path p1("testfile.txt");
  Path p2("..");
  Path p3("doesnotexist");

  IS_TRUE(p1.is_file());
  IS_FALSE(p1.is_directory());
  IS_TRUE(p2.is_directory());
  IS_FALSE(p2.is_file());
  IS_FALSE(p3.is_directory());
  IS_FALSE(p3.is_file());
  IS_FALSE(p3.is_symlink());
}

void test_mkdir()
{
  Path p1("foo");

  // Should not error
  p1.mkdir();

  assert_exception(new Pathie::ErrnoError(1), __FILE__, __LINE__, []()
  {
    Path p("parent/does/not/exist");
    p.mkdir();
  });


  Path p2("foo/bar/baz/blubb");
  p2.mktree();
  IS_TRUE(p2.is_directory());
  IS_TRUE(p2.parent().prune().is_directory());
}

void test_deletion()
{
  Path p1("kkfdkfkfkf");
  p1.mkdir();
  p1.rmdir();
  IS_FALSE(p1.exists());

  p1.touch();
  p1.unlink();
  IS_FALSE(p1.exists());

  p1.mkdir();
  p1.remove();
  IS_FALSE(p1.exists());

  p1.touch();
  p1.remove();
  IS_FALSE(p1.exists());

  Path p2("foo/xxx/yyy/zzz");
  Path p3("foo");
  p2.mktree();
  IS_TRUE(p2.exists());
  p3.rmtree();
  IS_FALSE(p3.exists());
}

void test_equality()
{
  Path p1("/foo/bar");
  Path p2("/foo/bar");
  Path p3("foo/bar");

  IS_TRUE(p1 == p1);
  IS_TRUE(p1 == p2);
  IS_TRUE(p2 == p1);
  IS_FALSE(p1 == p3);
  IS_FALSE(p3 == p1);
}

void test_glob()
{
  std::vector<Path> files1 = Path::glob("*.txt");
  std::vector<Path> files2 = Path::pwd().dglob("*.txt");

  std::sort(files1.begin(), files1.end());
  std::sort(files2.begin(), files2.end());

  EQUAL(2, files1.size());
  if (files1.size() > 0) {
    EQUAL("testfile.txt", files1[0]);
    EQUAL("tästfile.txt", files1[1]);
  }

  EQUAL(2, files2.size());
  if (files2.size() > 0) {
    EQUAL(Path::pwd().join("testfile.txt").str(), files2[0]);
    EQUAL(Path::pwd().join("tästfile.txt").str(), files2[1]);
  }
}

void test_fnmatch()
{
  Path p("/tmp/foo/bar.txt");

  IS_TRUE(p.fnmatch("/tmp/*"));
  IS_TRUE(p.fnmatch("/tmp/f?o/bar.txt"));
  IS_TRUE(p.fnmatch("/tmp/foo/*.txt"));
  IS_FALSE(p.fnmatch("/xxx/*"));
}

void test_native()
{
  Path p("foo/bar");
#if defined(_WIN32)
  EQUAL(utf8_to_utf16("foo\\bar"), p.native());
#elif defined(_PATHIE_UNIX)
  EQUAL("foo/bar", p.native());
#else
#error Unsupported system
#endif
}

void test_parent()
{
  Path p1("foo/bar/baz");
  Path p2("/foo/bar");
  Path p3("/foo");
  Path p4("/");

  EQUAL("foo/bar", p1.parent());
  EQUAL("/foo", p2.parent());
  EQUAL("/", p3.parent());
  EQUAL("/", p4.parent());

#ifdef _WIN32
  Path p5("C:/foo");
  Path p6("C:/");

  EQUAL("C:/", p5.parent());
  EQUAL("C:/", p6.parent());
#endif
}

void test_sub_ext()
{
  Path p1("foo/bar");
  Path p2("foo/bar.txt");

  EQUAL("foo/bar.xxx", p1.sub_ext(".xxx"));
  EQUAL("foo/bar.xxx", p1.sub_ext("xxx"));
  EQUAL("foo/bar.xxx", p2.sub_ext(".xxx"));
  EQUAL("foo/bar.xxx", p2.sub_ext("xxx"));
}

void test_symbolic_links()
{
  Path p1("foo/bar/baz");
  Path p2("foo/bla");
  Path p3("xxx");

  p1 = p1.absolute();
  p2 = p2.absolute();

  if (!p2.parent().is_directory())
    p2.parent().mkdir();

  if (!p1.parent().is_directory())
    p1.parent().mkdir();

  p1.touch();
  IS_TRUE(p1.exists());

#ifdef _WIN32
  try {
#endif
    p2.make_symlink(p1); // Link to absolute target
    IS_TRUE(p2.exists());
    IS_TRUE(p2.is_symlink());
    IS_FALSE(p1.is_symlink());

    EQUAL(Path::pwd().join("foo/bar/baz").str(), p2.readlink());
    p2.unlink();
    IS_FALSE(p2.is_symlink()); // deleted file

    p2.make_symlink(p3); //  Link to relative target
    IS_TRUE(p2.is_symlink());
    EQUAL("xxx", p2.readlink());
#ifdef _WIN32
  }
  catch(Pathie::WindowsError& err) {
    if (err.get_val() == ERROR_PRIVILEGE_NOT_HELD)
      std::cout << "Process is not allowed to create symlinks, ignoring test" << std::endl;
    else
      std::cout << FAIL << std::endl;
  }
#endif

  p2.parent().rmtree();
}

int main(int argc, char* argv[])
{
#ifndef _WIN32
  std::locale::global(std::locale(""));
#endif

  std::cout << "Cleaning." << std::endl;
  system("rake clean");

  s_testsettings = get_testsettings();

  test_constructor();
  test_sanitizer();
  test_root();
  test_relative_absolute();
  test_dirname_basename();
  test_extension();
  test_operators();
  test_prune();
  test_pwd();
  test_absolute();
  test_expand();
  test_stat();
  test_system_paths();
  test_splits();
  test_exists();
  test_stats();
  test_mkdir();
  test_deletion();
  test_equality();
  test_glob();
  test_fnmatch();
  test_native();
  test_parent();
  test_sub_ext();
  test_symbolic_links();

  return 0;
}
