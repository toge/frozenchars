#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace frozenchars::json::detail {

/// JSON-Crush で crush 結果と分割文字列を区切るデリミタ（制御文字 U+0001）
inline constexpr auto JSON_CRUSH_DELIMITER = char16_t{0x0001};

/**
 * @brief JSON-Crush の置換に使用可能な UTF-16 文字テーブル
 *
 * @details URI エンコードしても安全な文字を優先度順に集めた 222 要素の配列。
 * 前半で URI 非安全（=エンコードで長くなる）文字を降順に、後半で URI 安全文字を
 * 降順に格納する。'\\'(92) は特殊扱いのため除外する。
 */
inline constexpr auto replacement_characters_utf16 = [] {
  std::array<char16_t, 222> chars{};
  size_t idx = 0;
  auto is_unescaped = [](char16_t c) -> bool {
    // URI エンコード対象外のマーク文字（RFC 3986 の unreserved marks）
    constexpr char16_t unescaped[] = {u'-', u'_', u'.', u'!', u'~', u'*', u'\'', u'(', u')'};
    for (auto x : unescaped) if (x == c) return true;
    return false;
  };
  for (int i = 254; i >= 32; --i) {
    if (i == 92) continue;
    bool in_uri_safe = (i >= 48 && i <= 57) || (i >= 65 && i <= 90) || (i >= 97 && i <= 122) ||
                       is_unescaped(static_cast<char16_t>(i));
    if (!in_uri_safe) chars[idx++] = static_cast<char16_t>(i);
  }
  for (int i = 126; i > 0; --i) {
    bool in_uri_safe = (i >= 48 && i <= 57) || (i >= 65 && i <= 90) || (i >= 97 && i <= 122);
    in_uri_safe = in_uri_safe || is_unescaped(static_cast<char16_t>(i));
    if (in_uri_safe) chars[idx++] = static_cast<char16_t>(i);
  }
  return chars;
}();

/**
 * @brief UTF-8 バイト列から1つのコードポイントをデコードする
 *
 * @param input UTF-8 バイト列
 * @param index 開始位置。デコードしたバイト数だけ進める（1〜4バイト）
 * @return char32_t デコードしたコードポイント
 * @throw std::runtime_error 不正な UTF-8 シーケンスの場合
 */
inline constexpr auto decode_utf8 = [](std::string_view const input, size_t& index) -> char32_t {
  auto const lead = static_cast<uint8_t>(input[index]);
  // 1バイト（ASCII）
  if (lead < 0x80) { return static_cast<char32_t>(input[index++]); }
  // 2バイトシーケンス
  if ((lead & 0xE0) == 0xC0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    if ((c1 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 2;
    return (static_cast<char32_t>(lead & 0x1F) << 6) | static_cast<char32_t>(c1 & 0x3F);
  }
  // 3バイトシーケンス
  if ((lead & 0xF0) == 0xE0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    auto const c2 = static_cast<uint8_t>(input[index + 2]);
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 3;
    return (static_cast<char32_t>(lead & 0x0F) << 12) |
           (static_cast<char32_t>(c1 & 0x3F) << 6) |
           static_cast<char32_t>(c2 & 0x3F);
  }
  // 4バイトシーケンス
  if ((lead & 0xF8) == 0xF0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    auto const c2 = static_cast<uint8_t>(input[index + 2]);
    auto const c3 = static_cast<uint8_t>(input[index + 3]);
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
      throw std::runtime_error("invalid UTF-8");
    index += 4;
    return (static_cast<char32_t>(lead & 0x07) << 18) |
           (static_cast<char32_t>(c1 & 0x3F) << 12) |
           (static_cast<char32_t>(c2 & 0x3F) << 6) |
           static_cast<char32_t>(c3 & 0x3F);
  }
  throw std::runtime_error("invalid UTF-8");
};

/**
 * @brief UTF-8 文字列を UTF-16 文字列に変換する
 *
 * @param input UTF-8 バイト列
 * @return std::u16string 変換した UTF-16 文字列（BMP 外はサロゲートペアで表現）
 * @throw std::runtime_error 不正な UTF-8 シーケンスの場合
 */
inline constexpr auto utf8_to_utf16 = [](std::string_view const input) -> std::u16string {
  auto output = std::u16string{};
  output.reserve(input.size());
  size_t idx = 0;
  while (idx < input.size()) {
    auto const cp = decode_utf8(input, idx);
    if (cp < 0x10000) {
      output.push_back(static_cast<char16_t>(cp));
    } else {
      // BMP 外はサロゲートペアに分割

      output.push_back(static_cast<char16_t>(0xD800 + ((cp - 0x10000) >> 10)));
      output.push_back(static_cast<char16_t>(0xDC00 + ((cp - 0x10000) & 0x3FF)));
    }
  }
  return output;
};

/**
 * @brief UTF-16 文字列を UTF-8 文字列に変換する
 *
 * @param input UTF-16 文字列
 * @return std::string 変換した UTF-8 バイト列
 * @throw std::runtime_error 対になっていないサロゲートがある場合
 */
inline constexpr auto utf16_to_utf8 = [](std::u16string_view const input) -> std::string {
  auto output = std::string{};
  output.reserve(input.size());
  // 1つのコードポイントを UTF-8 バイト列として追記する
  auto append_utf8 = [&](char32_t cp) {
    if (cp < 0x80) {
      output.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
      output.push_back(static_cast<char>(0xC0 | (cp >> 6)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
      output.push_back(static_cast<char>(0xE0 | (cp >> 12)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
      output.push_back(static_cast<char>(0xF0 | (cp >> 18)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
  };
  for (size_t i = 0; i < input.size(); ++i) {
    auto const v = input[i];
    // 上位サロゲートなら次の下位サロゲートと合成してコードポイントを復元
    if (v >= 0xD800 && v <= 0xDBFF) {
      if (i + 1 >= input.size() || input[i + 1] < 0xDC00 || input[i + 1] > 0xDFFF)
        throw std::runtime_error("invalid UTF-16");
      auto const cp = 0x10000 + ((static_cast<char32_t>(v - 0xD800) << 10) | (input[i + 1] - 0xDC00));
      append_utf8(cp);
      ++i;
    } else {
      append_utf8(static_cast<char32_t>(v));
    }
  }
  return output;
};

} // namespace frozenchars::json::detail


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
};

/**
 * @brief 対になっていないサロゲートを含むかを判定する
 *
 * @tparam CharT 文字型
 * @param s 判定する文字列
 * @return bool 孤立した上位/下位サロゲートを含むなら true
 * @note char16_t 以外では常に false。char16_t では上位サロゲートの直後に
 * 下位サロゲートが続くこと、下位サロゲートが単独で現れないことを検証する。
 */
template <typename CharT>
[[nodiscard]] constexpr auto has_unmatched_surrogate(std::basic_string_view<CharT> const s) -> bool {
  if constexpr (std::same_as<CharT, char16_t>) {
    for (size_t i = 0; i < s.size(); ++i) {
      auto const c = static_cast<uint32_t>(s[i]);
      if (c >= 0xD800 && c <= 0xDBFF) {
        if (i + 1 >= s.size() || s[i + 1] < 0xDC00 || s[i + 1] > 0xDFFF) {
          return true;
        }
        ++i;
      } else if (c >= 0xDC00 && c <= 0xDFFF) {
        return true;
      }
    }
  }
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
 * @param data_end 置換対象とするデータ領域の終端位置
 * @return std::basic_string<CharT> 置換後の文字列
 */
template <typename CharT>
[[nodiscard]] constexpr auto replace_all_with_char(
    std::basic_string<CharT> str,
    std::basic_string_view<CharT> const from,
    CharT const to,
    size_t const data_end) -> std::basic_string<CharT> {
  auto end = data_end;
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::basic_string<CharT>::npos && pos + from.size() <= end) {
    str.replace(pos, from.size(), 1, to);
    end -= from.size() - 1;
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
 * @param data_end 候補を収集するデータ領域の終端位置
 * @return 重複を除去した圧縮候補（OrderedCandidate）の一覧
 * @details 長さ 2..max_len の全部分文字列をローリングハッシュで生成しソートして
 * 同一部分文字列をまとめ、重複せず2回以上出現するものを候補とする。
 */
template <typename CharT>
[[nodiscard]] constexpr auto build_initial_candidates(
    std::basic_string_view<CharT> const string, int64_t const max_len = 50,
    size_t const data_end = std::basic_string_view<CharT>::npos) {
  struct BucketEntry { uint64_t hash; size_t len; size_t pos; };
  auto const n = data_end == std::basic_string_view<CharT>::npos ? string.size() : data_end;
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
            std::basic_string<CharT>(sub), cnt
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
 * @param data_end 出現を数えるデータ領域の終端位置
 * @note 出現は非重複でカウントする（1回数えたら値の長さぶん進める）
 */
template <typename CharT>
constexpr void count_candidates(
    std::basic_string_view<CharT> const str,
    std::vector<OrderedCandidate<CharT>>& candidates,
    size_t const data_end) {
  for (auto& c : candidates) {
    c.count = 0;
    size_t pos = 0;
    std::basic_string_view<CharT> cv{c.value.data(), c.value.size()};
    while ((pos = str.find(cv, pos)) != std::basic_string_view<CharT>::npos && pos + cv.size() <= data_end) {
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
      auto const rest = std::basic_string_view<CharT>(s).substr(i);
      if (rest.starts_with(a)) {
        result.append(b); i += a.size();
      } else if (rest.starts_with(b)) {
        result.append(a); i += b.size();
      } else {
        result.push_back(s[i]); ++i;
      }
    }
    return result;
  };
  static_assert(std::same_as<CharT, char16_t>);
  static constexpr auto groups_data = [] {
    struct P { std::u16string_view a; std::u16string_view b; };  // 置換元(a)と置換先(b)のペア
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
  struct Pair { std::basic_string_view<CharT> a; std::basic_string_view<CharT> b; };  // 置換元(a)と置換先(b)のペア
  Pair const groups[] = {
    {to_view(groups_data[0].a), to_view(groups_data[0].b)},
    {to_view(groups_data[1].a), to_view(groups_data[1].b)},
    {to_view(groups_data[2].a), to_view(groups_data[2].b)},
    {to_view(groups_data[3].a), to_view(groups_data[3].b)},
    {to_view(groups_data[4].a), to_view(groups_data[4].b)},
  };
  auto str = std::basic_string<CharT>(input);
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
 * 反復する。置換はデータ領域（辞書開始位置より前）に限定し、末尾に追記した辞書
 * （「置換文字＋元部分文字列」）は以後不変とする。これにより復元時に辞書を
 * 一意に参照できる。
 */
template <typename CharT>
[[nodiscard]] constexpr auto js_crush_utf16(
    std::basic_string<CharT> string, int64_t const max_len = 50) -> JSCrushResult<CharT> {
  std::basic_string<CharT> split_string;
  size_t data_end = string.size();  // データ領域の終端（辞書開始位置）
  auto candidates = build_initial_candidates<CharT>(string, max_len, data_end);
  int replace_pos = static_cast<int>(replacement_characters_utf16.size());

  while (true) {
    // 現在の文字列（辞書含む）に出現する文字を記録し、未使用の置換文字を選ぶ
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
    // constexpr 評価の演算数を抑えるため、erase ではなく合格候補の新ベクタへ詰め直す
    int64_t rep_len = 1;
    int64_t delim_len = 1;
    size_t best_idx = 0;
    int64_t best_delta = 0;
    std::vector<OrderedCandidate<CharT>> kept;
    kept.reserve(candidates.size());
    for (auto& c : candidates) {
      int64_t delta = (c.count - 1) * static_cast<int64_t>(c.value.size()) - (c.count + 1) * rep_len;
      if (split_string.empty()) delta -= delim_len;
      if (delta <= 0) continue;
      if (delta > best_delta) { best_delta = delta; best_idx = kept.size(); }
      kept.push_back(std::move(c));
    }
    candidates = std::move(kept);
    if (best_delta <= 0 || candidates.empty()) break;  // これ以上縮まないなら終了

    // 最良候補をデータ領域のみで置換し、末尾に「置換文字＋元部分文字列」を辞書として付加
    // 既存の辞書は置換によるシフト後も内容不変で保持し、新しい辞書を末尾に追記する
    auto const& best_sub = candidates[best_idx].value;
    auto const dict_size = string.size() - data_end;
    string = replace_all_with_char<CharT>(string, best_sub, replace_char, data_end);
    data_end = string.size() - dict_size;
    string.push_back(replace_char);
    string.append(best_sub);
    data_end = string.size();
    split_string.insert(split_string.begin(), replace_char);

    // 残り候補も同じ置換を反映し、短くなりすぎ・重複したものを除いて再構築
    struct SeenEntry { std::basic_string<CharT> value; };
    std::vector<SeenEntry> seen;
    std::vector<OrderedCandidate<CharT>> next_cands;
    for (auto& c : candidates) {
      auto rewritten = replace_all_with_char<CharT>(c.value, best_sub, replace_char, c.value.size());
      if (rewritten.size() < 2) continue;
      bool found = false;
      for (auto const& s : seen) {
        if (s.value == rewritten) { found = true; break; }
      }
      if (!found) {
        seen.push_back({rewritten});
        next_cands.push_back({rewritten, 0});
      }
    }
    candidates = std::move(next_cands);
    count_candidates<CharT>(string, candidates, data_end);
  }
  return {std::move(string), std::move(split_string)};
}

} // namespace frozenchars::json::detail
