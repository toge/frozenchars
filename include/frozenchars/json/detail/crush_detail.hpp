#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "common.hpp"

namespace frozenchars::json::detail {

// -- RollingHash (Rabin-Karp) --
template <typename CharT>
struct RollingHash {
  std::basic_string_view<CharT> str;
  static constexpr uint64_t BASE = 1000003;

  [[nodiscard]] constexpr auto slice(size_t pos, size_t len) const -> uint64_t {
    uint64_t h = 0;
    for (size_t i = 0; i < len; ++i) h = h * BASE + static_cast<uint64_t>(str[pos + i]);
    return h;
  }
};

// -- OrderedCandidate --
template <typename CharT>
struct OrderedCandidate {
  std::basic_string<CharT> value;
  int64_t count;
  int64_t encoded_length;
};

// -- helpers --
template <typename CharT>
[[nodiscard]] constexpr auto encoded_uri_length(std::basic_string_view<CharT> const&) -> int64_t {
  return 1;
}

template <typename CharT>
[[nodiscard]] constexpr auto has_unmatched_surrogate(std::basic_string_view<CharT>) -> bool {
  return false;
}

template <typename CharT>
[[nodiscard]] constexpr auto greedy_non_overlapping_count(
    std::vector<size_t> const& positions, size_t len) -> int64_t {
  if (positions.empty()) return 0;
  int64_t cnt = 1;
  size_t end = positions[0] + len;
  for (size_t i = 1; i < positions.size(); ++i) {
    if (positions[i] >= end) { ++cnt; end = positions[i] + len; }
  }
  return cnt;
}

template <typename CharT>
[[nodiscard]] constexpr auto replace_all_with_char(
    std::basic_string<CharT> str,
    std::basic_string_view<CharT> const from,
    CharT const to) -> std::basic_string<CharT> {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::basic_string<CharT>::npos) {
    str.replace(pos, from.size(), 1, to);
    pos += 1;
  }
  return str;
}

// -- build_initial_candidates --
template <typename CharT>
[[nodiscard]] constexpr auto build_initial_candidates(
    std::basic_string_view<CharT> const string, int64_t const max_len = 50) {
  struct BucketEntry { uint64_t hash; size_t len; size_t pos; };
  auto const n = string.size();
  std::vector<BucketEntry> all_entries;
  RollingHash<CharT> const hasher{string};
  for (size_t len = 2; len < static_cast<size_t>(max_len) && len <= n; ++len) {
    for (size_t i = 0; i + len <= n; ++i) {
      all_entries.push_back({hasher.slice(i, len), len, i});
    }
  }
  std::sort(all_entries.begin(), all_entries.end(),
    [](auto const& a, auto const& b) {
      if (a.hash != b.hash) return a.hash < b.hash;
      if (a.len != b.len) return a.len < b.len;
      return a.pos < b.pos;
    });
  std::vector<OrderedCandidate<CharT>> candidates;
  for (size_t gi = 0; gi < all_entries.size(); ) {
    auto const h = all_entries[gi].hash;
    auto const len = all_entries[gi].len;
    if (gi + 1 < all_entries.size() &&
        all_entries[gi + 1].hash == h && all_entries[gi + 1].len == len) {
      std::vector<size_t> positions;
      while (gi < all_entries.size() && all_entries[gi].hash == h && all_entries[gi].len == len) {
        positions.push_back(all_entries[gi].pos);
        ++gi;
      }
      auto const cnt = greedy_non_overlapping_count<CharT>(positions, len);
      if (cnt > 1) {
        auto const sub = string.substr(positions[0], len);
        if (!has_unmatched_surrogate(sub)) {
          candidates.push_back({
            std::basic_string<CharT>(sub), cnt,
            encoded_uri_length<CharT>(std::basic_string_view<CharT>(sub))
          });
        }
      }
    } else {
      ++gi;
    }
  }
  std::vector<OrderedCandidate<CharT>> deduped;
  for (auto const& c : candidates) {
    bool found = false;
    for (auto const& d : deduped) {
      if (d.value == c.value) { found = true; break; }
    }
    if (!found) deduped.push_back(c);
  }
  return deduped;
}

// -- count_candidates --
template <typename CharT>
constexpr void count_candidates(
    std::basic_string_view<CharT> const str,
    std::vector<OrderedCandidate<CharT>>& candidates) {
  for (auto& c : candidates) {
    c.count = 0;
    size_t pos = 0;
    while ((pos = str.find(c.value, pos)) != std::basic_string_view<CharT>::npos) {
      ++c.count;
      pos += c.value.size();
    }
  }
}

// -- json_crush_swap --
template <typename CharT>
[[nodiscard]] constexpr auto json_crush_swap(
    std::basic_string_view<CharT> const input, bool const forward = true) -> std::basic_string<CharT> {
  auto apply = [](std::basic_string<CharT> s,
                   std::basic_string_view<CharT> a,
                   std::basic_string_view<CharT> b) -> std::basic_string<CharT> {
    auto result = std::basic_string<CharT>{};
    result.reserve(s.size());
    for (size_t i = 0; i < s.size(); ) {
      if (i + a.size() <= s.size() && s.substr(i, a.size()) == a) {
        result.append(b); i += a.size();
      } else if (i + b.size() <= s.size() && s.substr(i, b.size()) == b) {
        result.append(a); i += b.size();
      } else {
        result.push_back(s[i]); ++i;
      }
    }
    return result;
  };
  static_assert(std::same_as<CharT, char16_t>);
  static constexpr auto groups_data = [] {
    struct P { std::u16string_view a; std::u16string_view b; };
    return std::array<P, 5>{P{u"\"" , u"'"},
                            P{u"':", u"!"},
                            P{u",'", u"~"},
                            P{u"}" , u")"},
                            P{u"{" , u"("}};
  }();
  auto to_view = [](std::u16string_view u16) -> std::basic_string_view<CharT> {
    if constexpr (std::same_as<CharT, char16_t>) return u16;
    else return {};
  };
  struct Pair { std::basic_string_view<CharT> a; std::basic_string_view<CharT> b; };
  Pair const groups[] = {
    {to_view(groups_data[0].a), to_view(groups_data[0].b)},
    {to_view(groups_data[1].a), to_view(groups_data[1].b)},
    {to_view(groups_data[2].a), to_view(groups_data[2].b)},
    {to_view(groups_data[3].a), to_view(groups_data[3].b)},
    {to_view(groups_data[4].a), to_view(groups_data[4].b)},
  };
  auto str = std::basic_string<CharT>{input};
  if (forward) {
    for (auto const& g : groups) str = apply(str, g.a, g.b);
  } else {
    for (int i = 4; i >= 0; --i) str = apply(str, groups[i].a, groups[i].b);
  }
  return str;
}

// -- js_crush_utf16 result --
template <typename CharT>
struct JSCrushResult {
  std::basic_string<CharT> crushed;
  std::basic_string<CharT> split;
};

// -- js_crush_utf16 core --
template <typename CharT>
[[nodiscard]] constexpr auto js_crush_utf16(
    std::basic_string<CharT> string, int64_t const max_len = 50) -> JSCrushResult<CharT> {
  std::basic_string<CharT> split_string;
  auto candidates = build_initial_candidates<CharT>(string, max_len);
  int replace_pos = static_cast<int>(replacement_characters_utf16.size());

  while (true) {
    std::bitset<65536> present;
    for (auto c : string) {
      if (static_cast<uint16_t>(c) < 65536) present.set(static_cast<uint16_t>(c));
    }
    CharT replace_char = 0;
    while (replace_pos > 0) {
      auto const c = replacement_characters_utf16[--replace_pos];
      if (!present.test(static_cast<uint16_t>(c))) { replace_char = static_cast<CharT>(c); break; }
    }
    if (replace_char == 0) break;

    int64_t rep_len = 1;
    int64_t delim_len = 1;
    size_t best_idx = 0;
    int64_t best_delta = 0;
    auto it = candidates.begin();
    while (it != candidates.end()) {
      int64_t delta = (it->count - 1) * it->encoded_length - (it->count + 1) * rep_len;
      if (split_string.empty()) delta -= delim_len;
      if (delta <= 0) {
        it = candidates.erase(it);
      } else {
        if (delta > best_delta) { best_delta = delta; best_idx = std::distance(candidates.begin(), it); }
        ++it;
      }
    }
    if (best_delta <= 0 || candidates.empty()) break;

    auto const& best_sub = candidates[best_idx].value;
    string = replace_all_with_char<CharT>(string, best_sub, replace_char);
    string.push_back(replace_char);
    string.append(best_sub);
    split_string.insert(split_string.begin(), replace_char);

    struct SeenEntry { std::basic_string<CharT> value; };
    std::vector<SeenEntry> seen;
    std::vector<OrderedCandidate<CharT>> next_cands;
    for (auto& c : candidates) {
      auto rewritten = replace_all_with_char<CharT>(c.value, best_sub, replace_char);
      if (rewritten.size() < 2) continue;
      bool found = false;
      for (auto const& s : seen) {
        if (s.value == rewritten) { found = true; break; }
      }
      if (!found) {
        seen.push_back({rewritten});
        next_cands.push_back({rewritten, 0, encoded_uri_length<CharT>(rewritten)});
      }
    }
    candidates = std::move(next_cands);
    count_candidates<CharT>(string, candidates);
  }
  return {std::move(string), std::move(split_string)};
}

} // namespace frozenchars::json::detail
