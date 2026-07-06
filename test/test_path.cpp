#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;
using namespace frozenchars::path;

TEST_CASE("path::dirname", "[path]") {
  REQUIRE(dirname(""_fs).sv() == ".");
  STATIC_CHECK(dirname("foo"_fs).sv() == ".");
  STATIC_CHECK(dirname("/"_fs).sv() == "/");
  STATIC_CHECK(dirname("/usr"_fs).sv() == "/");
  STATIC_CHECK(dirname("/usr/bin"_fs).sv() == "/usr");
  STATIC_CHECK(dirname("/usr/bin/gcc"_fs).sv() == "/usr/bin");
  STATIC_CHECK(dirname("usr/local/bin"_fs).sv() == "usr/local");
}

TEST_CASE("path::basename", "[path]") {
  STATIC_CHECK(basename(""_fs).sv() == "");
  STATIC_CHECK(basename("foo"_fs).sv() == "foo");
  STATIC_CHECK(basename("/"_fs).sv() == "");
  STATIC_CHECK(basename("/usr"_fs).sv() == "usr");
  STATIC_CHECK(basename("/usr/bin"_fs).sv() == "bin");
  STATIC_CHECK(basename("usr/local/bin"_fs).sv() == "bin");
  STATIC_CHECK(basename("foo/"_fs).sv() == "");
}

TEST_CASE("path::extension", "[path]") {
  STATIC_CHECK(extension(""_fs).sv() == "");
  STATIC_CHECK(extension("foo"_fs).sv() == "");
  STATIC_CHECK(extension("foo.txt"_fs).sv() == ".txt");
  STATIC_CHECK(extension("foo.bar.txt"_fs).sv() == ".txt");
  STATIC_CHECK(extension(".gitignore"_fs).sv() == "");
  STATIC_CHECK(extension("/usr/bin/foo.tar.gz"_fs).sv() == ".gz");
  STATIC_CHECK(extension("noext"_fs).sv() == "");
}

TEST_CASE("path::stem", "[path]") {
  STATIC_CHECK(stem(""_fs).sv() == "");
  STATIC_CHECK(stem("foo"_fs).sv() == "foo");
  STATIC_CHECK(stem("foo.txt"_fs).sv() == "foo");
  STATIC_CHECK(stem("foo.bar.txt"_fs).sv() == "foo.bar");
  STATIC_CHECK(stem(".gitignore"_fs).sv() == ".gitignore");
  STATIC_CHECK(stem("/usr/bin/foo.tar.gz"_fs).sv() == "foo.tar");
  STATIC_CHECK(stem("noext"_fs).sv() == "noext");
}

TEST_CASE("path::join", "[path]") {
  STATIC_CHECK(join("usr"_fs, "bin"_fs).sv() == "usr/bin");
  STATIC_CHECK(join("/usr"_fs, "bin"_fs).sv() == "/usr/bin");
  STATIC_CHECK(join("/usr/"_fs, "bin"_fs).sv() == "/usr/bin");
  STATIC_CHECK(join("usr"_fs, "/bin"_fs).sv() == "/bin");
  STATIC_CHECK(join("a"_fs, "b"_fs, "c"_fs).sv() == "a/b/c");
  STATIC_CHECK(join("a/"_fs, "/b"_fs).sv() == "/b");
  STATIC_CHECK(join("a"_fs, ""_fs).sv() == "a");
  STATIC_CHECK(join(""_fs, "b"_fs).sv() == "b");
  STATIC_CHECK(join(""_fs, ""_fs).sv() == "");
}
