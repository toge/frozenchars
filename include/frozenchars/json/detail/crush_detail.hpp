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

/**
 * @brief Rabin-Karp 法による部分文字列ハッシュ計算器
 *
 * @tparam CharT 文字型
 */
template <typename CharT>
struct RollingHash {
  std::basic_string_view<CharT> str;        ///< ハッシュ対象の文字列
  static constexpr uint64_t BASE = 1000003;  ///< 多項式ハッシュの基数（素数）

  /**
   * @brief 指定範囲の部分文字列の多項式ハッシュを計算する
   *
   * @param pos 開始位置
   * @param len 長さ
   * @return uint64_t 計算したハッシュ値
   */
  [[nodiscard]] constexpr auto slice(size_t pos, size_t len) const -> uint64_t {
    uint64_t h = 0;
    for (size_t i = 0; i < len; ++i) h = h * BASE + static_cast<uint64_t>(str[pos + i]);
    return h;
  }
};

/**
 * @brief 圧縮候補となる繰り返し部分文字列とその評価情報
 *
 * @tparam CharT 文字型
 */
template <typename CharT>
struct OrderedCandidate {
  std::basic_string<CharT> value;  ///< 候補となる部分文字列
  int64_t count;                   ///< 出現回数
  int64_t encoded_length;          ///< 置換文字1つに符号化した際の長さ（URI エンコード想定）
};

/**
 * @brief 部分文字列を置換文字1つに符号化した際の長さを返す
 *
 * @tparam CharT 文字型
 * @return int64_t 常に 1（置換文字1文字ぶん）
 */
template <typename CharT>
[[nodiscard]] constexpr auto encoded_uri_length(std::basic_string_view<CharT> const&) -> int64_t {
  return 1;
}

/**
 * @brief 対になっていないサロゲートを含むかを判定する
 *
 * @tparam CharT 文字型
 * @return bool 常に false（現状は判定を行わないスタブ）
 */
template <typename CharT>
[[nodiscard]] constexpr auto has_unmatched_surrogate(std::basic_string_view<CharT>) -> bool {
  return false;
}

/**
 * @brief 重複しない出現位置を貪欲に数える
 *
 * @tparam CharT 文字型
 * @param positions 昇順に並んだ出現開始位置の一覧
 * @param len 部分文字列の長さ
 * @return int64_t 直前の出現と重ならないように選んだ最大の出現回数
 */
template <typename CharT>
[[nodiscard]] constexpr auto greedy_non_overlapping_count(std::vector<size_t> const& positions, size_t len) -> int64_t {
  if (positions.empty()) return 0;
  int64_t cnt = 1;
  size_t end = positions[0] + len;
  for (size_t i = 1; i < positions.size(); ++i) {
    if (positions[i] >= end) { ++cnt; end = positions[i] + len; }
  }
  return cnt;
}

/**
 * @brief 文字列中の部分文字列を全て1文字に置換する
 *
 * @tparam CharT 文字型
 * @param str 対象文字列（値渡し）
 * @param from 置換対象の部分文字列
 * @param to 置換後の1文字
 * @return std::basic_string<CharT> 置換後の文字列
 */
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

/**
 * @brief 初期の圧縮候補（複数回出現する部分文字列）を列挙する
 *
 * @tparam CharT 文字型
 * @param string 対象文字列
 * @param max_len 候補として考慮する部分文字列の最大長
 * @return 重複を除去した圧縮候補（OrderedCandidate）の一覧
 * @details 長さ 2..max_len の全部分文字列をローリングハッシュで生成しソートして
 * 同一部分文字列をまとめ、重複せず2回以上出現するものを候補とする。
 */
template <typename CharT>
[[nodiscard]] constexpr auto build_initial_candidates(
    std::basic_string_view<CharT> const string, int64_t const max_len = 50) {
  struct BucketEntry { uint64_t hash; size_t len; size_t pos; };
  auto const n = string.size();
  std::vector<BucketEntry> all_entries;
  RollingHash<CharT> const hasher{string};
  // 長さごとに全部分文字列のハッシュ・長さ・位置を収集
  for (size_t len = 2; len < static_cast<size_t>(max_len) && len <= n; ++len) {
    for (size_t i = 0; i + len <= n; ++i) {
      all_entries.push_back({hasher.slice(i, len), len, i});
    }
  }
  // ハッシュ→長さ→位置の順にソートし、同一部分文字列を隣接させる
  std::sort(all_entries.begin(), all_entries.end(),
    [](auto const& a, auto const& b) {
      if (a.hash != b.hash) return a.hash < b.hash;
      if (a.len != b.len) return a.len < b.len;
      return a.pos < b.pos;
    });
  // 同一ハッシュ・同一長のグループごとに出現位置をまとめて候補化
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
  // 同一値の候補を除去する
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

/**
 * @brief 各候補の現在の文字列中での出現回数を数え直す
 *
 * @tparam CharT 文字型
 * @param str 対象文字列
 * @param candidates 出現回数を更新する候補一覧（count を書き換える）
 * @note 出現は非重複でカウントする（1回数えたら値の長さぶん進める）
 */
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

/**
 * @brief JSON 構造文字と短い記号の相互置換を行う
 *
 * @tparam CharT 文字型（char16_t のみ対応）
 * @param input 対象文字列
 * @param forward true で圧縮方向（構造文字→記号）、false で復元方向
 * @return std::basic_string<CharT> 置換後の文字列
 * @details `"`↔`'`, `':`↔`!`, `,'`↔`~`, `}`↔`)`, `{`↔`(` の5組を、圧縮時は先頭から、
 * 復元時は逆順に適用する。各組は互いに入れ替える（値がぶつからないよう swap する）。
 */
template <typename CharT>
[[nodiscard]] constexpr auto json_crush_swap(
    std::basic_string_view<CharT> const input, bool const forward = true) -> std::basic_string<CharT> {
  // 文字列 s 中の a と b を相互に入れ替える
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

/**
 * @brief JS-Crush の圧縮結果
 *
 * @tparam CharT 文字型
 */
template <typename CharT>
struct JSCrushResult {
  std::basic_string<CharT> crushed;  ///< 圧縮後の本体文字列（末尾に置換文字＋元部分文字列が付く）
  std::basic_string<CharT> split;    ///< 復元に使う置換文字の並び
};

/**
 * @brief JS-Crush アルゴリズムで文字列を圧縮する
 *
 * @tparam CharT 文字型
 * @param string 圧縮対象の文字列（値渡し、内部で書き換える）
 * @param max_len 候補とする部分文字列の最大長
 * @return JSCrushResult<CharT> 圧縮本体と分割文字列
 * @details 未使用の置換文字を選び、圧縮効果（delta）が最大の候補を1文字に置換する処理を
 * 反復する。置換のたびに候補を書き換えて出現回数を数え直し、効果が無くなったら終了する。
 */
template <typename CharT>
[[nodiscard]] constexpr auto js_crush_utf16(
    std::basic_string<CharT> string, int64_t const max_len = 50) -> JSCrushResult<CharT> {
  std::basic_string<CharT> split_string;
  auto candidates = build_initial_candidates<CharT>(string, max_len);
  int replace_pos = static_cast<int>(replacement_characters_utf16.size());

  while (true) {
    // 現在の文字列に出現する文字を記録し、未使用の置換文字を選ぶ
    std::bitset<65536> present;
    for (auto c : string) {
      if (static_cast<uint16_t>(c) < 65536) present.set(static_cast<uint16_t>(c));
    }
    CharT replace_char = 0;
    while (replace_pos > 0) {
      auto const c = replacement_characters_utf16[--replace_pos];
      if (!present.test(static_cast<uint16_t>(c))) { replace_char = static_cast<CharT>(c); break; }
    }
    if (replace_char == 0) break;  // 使える置換文字が尽きたら終了

    // 圧縮効果 delta = (出現回数-1)*符号長 - (出現回数+1)*置換長 が最大の候補を探す
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
    if (best_delta <= 0 || candidates.empty()) break;  // これ以上縮まないなら終了

    // 最良候補を置換文字に置き換え、末尾に「置換文字＋元部分文字列」を辞書として付加
    auto const& best_sub = candidates[best_idx].value;
    string = replace_all_with_char<CharT>(string, best_sub, replace_char);
    string.push_back(replace_char);
    string.append(best_sub);
    split_string.insert(split_string.begin(), replace_char);

    // 残り候補も同じ置換を反映し、短くなりすぎ・重複したものを除いて再構築
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
