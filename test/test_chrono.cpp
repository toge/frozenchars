#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"
#include "frozenchars/chrono.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;
using namespace std::chrono;

// ========== Parse ISO Date ==========

TEST_CASE("parse_iso_date - NTTP") {
  STATIC_CHECK(parse_iso_date<"2026-07-04"_fs>() == year{2026}/7/4);
  STATIC_CHECK(parse_iso_date<"2000-01-01"_fs>() == year{2000}/1/1);
  STATIC_CHECK(parse_iso_date<"2024-02-29"_fs>() == year{2024}/2/29);
  STATIC_CHECK(parse_iso_date<"1999-12-31"_fs>() == year{1999}/12/31);
}

TEST_CASE("parse_iso_date - runtime constexpr") {
  constexpr auto d1 = parse_iso_date("2026-07-04"_fs);
  STATIC_CHECK(d1 == year{2026}/7/4);

  constexpr auto d2 = parse_iso_date("2001-09-11"_fs);
  STATIC_CHECK(d2 == year{2001}/9/11);
}

// ========== Parse ISO Datetime ==========

TEST_CASE("parse_iso_datetime - UTC") {
  STATIC_CHECK(parse_iso_datetime<"2026-07-04T14:30:00Z"_fs>()
               == sys_days{year{2026}/7/4} + hours{14} + minutes{30});

  STATIC_CHECK(parse_iso_datetime<"2026-07-04T14:30:00"_fs>()
               == sys_days{year{2026}/7/4} + hours{14} + minutes{30});
}

TEST_CASE("parse_iso_datetime - timezone offset") {
  // +09:00 -> subtract 9h
  constexpr auto with_offset = parse_iso_datetime<"2026-07-04T14:30:00+09:00"_fs>();
  constexpr auto utc_ref = parse_iso_datetime<"2026-07-04T05:30:00Z"_fs>();
  STATIC_CHECK(with_offset == utc_ref);

  // -05:00 -> add 5h
  constexpr auto neg_offset = parse_iso_datetime<"2026-07-04T10:00:00-05:00"_fs>();
  constexpr auto utc_ref2 = parse_iso_datetime<"2026-07-04T15:00:00Z"_fs>();
  STATIC_CHECK(neg_offset == utc_ref2);
}

TEST_CASE("parse_iso_datetime - runtime constexpr") {
  constexpr auto dt = parse_iso_datetime("2024-01-15T09:05:01Z"_fs);
  STATIC_CHECK(dt == sys_days{year{2024}/1/15} + hours{9} + minutes{5} + seconds{1});
}

// ========== Parse __DATE__ macro ==========

TEST_CASE("parse_date_macro - known formats") {
  // "Jul  4 2026" -- 2 spaces before single-digit day
  STATIC_CHECK(parse_date_macro<"Jul  4 2026"_fs>() == year{2026}/7/4);
  // "Sep 14 2026" -- 1 space before 2-digit day
  STATIC_CHECK(parse_date_macro<"Sep 14 2026"_fs>() == year{2026}/9/14);
  // "Dec 31 2024"
  STATIC_CHECK(parse_date_macro<"Dec 31 2024"_fs>() == year{2024}/12/31);
}

TEST_CASE("parse_date_macro - runtime constexpr") {
  constexpr auto d = parse_date_macro("Jan 15 2024"_fs);
  STATIC_CHECK(d == year{2024}/1/15);
}

// ========== Compilation Timestamp ==========

TEST_CASE("compilation_timestamp - is valid sys_seconds") {
  constexpr auto ts = compilation_timestamp();
  // Must be a valid time_point (not at epoch)
  STATIC_CHECK(ts > sys_seconds{});
  STATIC_CHECK(ts < sys_days{year{2100}/1/1});

  // With offset: JST -> UTC
  constexpr auto ts_jst = compilation_timestamp(minutes{540});
  STATIC_CHECK(ts_jst == ts - minutes{540});
}

// ========== Format ISO Date ==========

TEST_CASE("format_iso_date - basic") {
  STATIC_CHECK(format_iso_date(year{2026}/7/4) == "2026-07-04"_fs);
  STATIC_CHECK(format_iso_date(year{2000}/1/1) == "2000-01-01"_fs);
  STATIC_CHECK(format_iso_date(year{2024}/12/31) == "2024-12-31"_fs);
}

TEST_CASE("format_iso_date - runtime") {
  auto const ymd = year{1999}/12/31;
  REQUIRE(format_iso_date(ymd) == "1999-12-31"_fs);
}

// ========== Format ISO Datetime ==========

TEST_CASE("format_iso_datetime - basic") {
  STATIC_CHECK(format_iso_datetime(sys_days{year{2026}/7/4} + hours{14} + minutes{30})
               == "2026-07-04T14:30:00Z"_fs);
  STATIC_CHECK(format_iso_datetime(sys_days{year{2000}/1/1} + hours{0} + minutes{0})
               == "2000-01-01T00:00:00Z"_fs);
}

TEST_CASE("format_iso_datetime - runtime") {
  auto const tp = sys_days{year{2024}/12/31} + hours{23} + minutes{59} + seconds{59};
  REQUIRE(format_iso_datetime(tp) == "2024-12-31T23:59:59Z"_fs);
}

// ========== Roundtrip ==========

TEST_CASE("roundtrip date") {
  constexpr auto ymd = year{2026}/7/4;
  constexpr auto formatted = format_iso_date(ymd);
  constexpr auto parsed = parse_iso_date(formatted);
  STATIC_CHECK(parsed == ymd);
}

TEST_CASE("roundtrip datetime") {
  constexpr auto tp = sys_days{year{2026}/7/4} + hours{14} + minutes{30} + seconds{5};
  constexpr auto formatted = format_iso_datetime(tp);
  // formatted == "2026-07-04T14:30:05Z"
  constexpr auto parsed = parse_iso_datetime(formatted);
  STATIC_CHECK(parsed == tp);
}
