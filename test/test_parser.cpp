#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

template <auto Src, typename Expected>
inline constexpr bool parse_to_tuple_is_v =
  std::is_same_v<typename decltype(frozenchars::parse_to_tuple<Src>())::type, Expected>;

TEST_CASE("parser all") {
  // 1. 単純な型の optional 対応
  {
    using T = typename decltype(parse_to_tuple<"int, int?"_fs>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, std::optional<int>>>);
  }

  // 2. 複数の optional
  {
    using T = typename decltype(parse_to_tuple<"int?, string?, bool"_fs>())::type;
    static_assert(std::is_same_v<T, std::tuple<std::optional<int>, std::optional<std::string>, bool>>);
  }

  // 3. 入れ子構造の optional 対応 ([...]? 形式)
  {
    using T = typename decltype(parse_to_tuple<"int, [int, int]?, string"_fs>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, std::optional<std::tuple<int, int>>, std::string>>);
  }

  // 4. 入れ子の中身が optional
  {
    using T = typename decltype(parse_to_tuple<"int, [int?, int], string?"_fs>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, std::tuple<std::optional<int>, int>, std::optional<std::string>>>);
  }

  // 5. 空白と ? の組み合わせ
  {
    using T = typename decltype(parse_to_tuple<" int ? , [ int , int ] ? , string "_fs>())::type;
    static_assert(std::is_same_v<T, std::tuple<std::optional<int>, std::optional<std::tuple<int, int>>, std::string>>);
  }
}

TEST_CASE("parser voids") {
  {
    auto constexpr src = ""_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<>>);
  }

  {
    auto constexpr src = ","_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void>>);
  }

  {
    auto constexpr src = ",,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void>>);
  }

  {
    auto constexpr src = ",,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void>>);
  }

  {
    auto constexpr src = ",,,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void, void>>);
  }

  {
    auto constexpr src = ",,,,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void, void, void>>);
  }

  {
    auto constexpr src = ",,,,,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void, void, void, void>>);
  }

  {
    auto constexpr src = ",,,,,,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void, void, void, void, void>>);
  }

  {
    auto constexpr src = ",,,,,,,,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, void, void, void, void, void, void, void, void>>);
  }

  {
    auto constexpr src = "int,"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, void>>);
  }

  {
    auto constexpr src = ",int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<void, int>>);
  } 
}

TEST_CASE("parser ints") {
  {
    auto constexpr src = "int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int>>);
  }

  {
    auto constexpr src = "int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int>>);
  }

  {
    auto constexpr src = "int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int>>);
  }

  {
    auto constexpr src = "int,int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int, int>>);
  }

  {
    auto constexpr src = "int,int,int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int, int, int>>);
  }

  {
    auto constexpr src = "int,int,int,int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int, int, int, int>>);
  }

  {
    auto constexpr src = "int,int,int,int,int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int, int, int, int, int>>);
  }

  {
    auto constexpr src = "int,int,int,int,int,int,int,int"_fs;
    using T = decltype(frozenchars::parse_to_tuple<src>())::type;
    static_assert(std::is_same_v<T, std::tuple<int, int, int, int, int, int, int, int>>);
  }
}

TEST_CASE("parser scalar aliases and mixed types") {
  static_assert(parse_to_tuple_is_v<"void"_fs, std::tuple<void>>);

  static_assert(parse_to_tuple_is_v<
    "bool,char,int,uint,long,ulong,float,double"_fs,
    std::tuple<bool, char, int, unsigned int, long, unsigned long, float, double>>);

  static_assert(parse_to_tuple_is_v<
    "str,sv,sz,int8,uint16,int32,uint64,void"_fs,
    std::tuple<std::string, std::string_view, std::size_t, std::int8_t, std::uint16_t, std::int32_t, std::uint64_t, void>>);

  static_assert(parse_to_tuple_is_v<
    "unsigned, string, string_view, size_t"_fs,
    std::tuple<unsigned int, std::string, std::string_view, std::size_t>>);
}

TEST_CASE("parser optional aliases and spacing") {
  static_assert(parse_to_tuple_is_v<
    "bool?,char?,int?,unsigned?,long?,str?,sv?,sz?"_fs,
    std::tuple<
      std::optional<bool>,
      std::optional<char>,
      std::optional<int>,
      std::optional<unsigned int>,
      std::optional<long>,
      std::optional<std::string>,
      std::optional<std::string_view>,
      std::optional<std::size_t>>>);

  static_assert(parse_to_tuple_is_v<
    " int8 ? , uint16 ? , int32 ? , uint64 ? "_fs,
    std::tuple<
      std::optional<std::int8_t>,
      std::optional<std::uint16_t>,
      std::optional<std::int32_t>,
      std::optional<std::uint64_t>>>);
}

TEST_CASE("parser nested tuples and optional nesting") {
  static_assert(parse_to_tuple_is_v<"[]"_fs, std::tuple<std::tuple<>>>);

  static_assert(parse_to_tuple_is_v<"[void]"_fs, std::tuple<std::tuple<void>>>);

  static_assert(parse_to_tuple_is_v<
    "[ ] ? , int"_fs,
    std::tuple<std::optional<std::tuple<>>, int>>);

  static_assert(parse_to_tuple_is_v<
    "[int, string], bool"_fs,
    std::tuple<std::tuple<int, std::string>, bool>>);

  static_assert(parse_to_tuple_is_v<
    "[int, [bool, char], string]"_fs,
    std::tuple<std::tuple<int, std::tuple<bool, char>, std::string>>>);

  static_assert(parse_to_tuple_is_v<
    "[int, [bool?, [char, double]?], string?]"_fs,
    std::tuple<
      std::tuple<
        int,
        std::tuple<std::optional<bool>, std::optional<std::tuple<char, double>>>,
        std::optional<std::string>>>>);

  static_assert(parse_to_tuple_is_v<
    "[int, int]?, [bool, bool?], string, [char]?"_fs,
    std::tuple<
      std::optional<std::tuple<int, int>>,
      std::tuple<bool, std::optional<bool>>,
      std::string,
      std::optional<std::tuple<char>>>>);
}

TEST_CASE("parser void holes and whitespace") {
  static_assert(parse_to_tuple_is_v<
    "int,,string"_fs,
    std::tuple<int, void, std::string>>);

  static_assert(parse_to_tuple_is_v<
    ", [int, string], , bool?"_fs,
    std::tuple<void, std::tuple<int, std::string>, void, std::optional<bool>>>);

  static_assert(parse_to_tuple_is_v<
    "[int,], string"_fs,
    std::tuple<std::tuple<int, void>, std::string>>);

  static_assert(parse_to_tuple_is_v<
    "[ , int], string"_fs,
    std::tuple<std::tuple<void, int>, std::string>>);

  static_assert(parse_to_tuple_is_v<
    " [ int , [ bool , char ? ] , string_view ] ? , double "_fs,
    std::tuple<
      std::optional<std::tuple<int, std::tuple<bool, std::optional<char>>, std::string_view>>,
      double>>);
}

TEST_CASE("parser maximum arity mixed cases") {
  static_assert(parse_to_tuple_is_v<
    "bool,[char,int?],,string?,[float,double]?,sv,sz,uint64"_fs,
    std::tuple<
      bool,
      std::tuple<char, std::optional<int>>,
      void,
      std::optional<std::string>,
      std::optional<std::tuple<float, double>>,
      std::string_view,
      std::size_t,
      std::uint64_t>>);

  static_assert(parse_to_tuple_is_v<
    "[int8,uint8],[int16,uint16]?,[int32,uint32],[int64,uint64]?,bool?,char?,float?,double?"_fs,
    std::tuple<
      std::tuple<std::int8_t, std::uint8_t>,
      std::optional<std::tuple<std::int16_t, std::uint16_t>>,
      std::tuple<std::int32_t, std::uint32_t>,
      std::optional<std::tuple<std::int64_t, std::uint64_t>>,
      std::optional<bool>,
      std::optional<char>,
      std::optional<float>,
      std::optional<double>>>);
}
