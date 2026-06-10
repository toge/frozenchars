#include <array>
#include <cstddef>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "frozenchars.hpp"

#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 14)
#define MAYBE_CONSTEXPR constexpr
#else
#define MAYBE_CONSTEXPR const
#endif

#if defined(__has_include) && __has_include(<glaze/json.hpp>)
#define HAS_GLAZE 1
#include <glaze/json.hpp>
#else
#define HAS_GLAZE 0
#endif

#if defined(FROZENCHARS_ENABLE_INJA)
#define HAS_INJA 1
#else
#define HAS_INJA 0
#endif

using namespace frozenchars;
using namespace frozenchars::literals;

namespace {

void print_section(std::string_view title) {
  std::cout << "\n=== " << title << " ===\n";
}

// 1. HTTP ステータスコード -> メッセージ
// before: static const std::unordered_map<std::string, std::string> kStatus = { ... };
// after : キー集合が型に昇格、typo = コンパイルエラー、ルックアップ O(1) かつ 1〜2 命令
void use_case_1_http_status() {
  print_section("1. HTTP status -> message (replaces static unordered_map)");

  static MAYBE_CONSTEXPR frozen_map<std::string_view,
    "200"_fs, "404"_fs, "500"_fs> kStatus{
    "OK", "Not Found", "Internal Server Error"
  };

  for (auto key : std::array<std::string_view, 3>{"200", "404", "500"}) {
    auto const v = kStatus.get(key);
    std::cout << "  " << key << " -> " << (v ? *v : std::string_view{"(unknown)"}) << '\n';
  }
  std::cout << "  418 -> " << (kStatus.get("418") ? "found" : "(unknown)") << '\n';
}

// 2. 環境名 -> URL (文字列 switch の置換)
// before: if (env == "dev") ... else if (env == "stg") ...
// after : default 不要、ルックアップが O(1)
void use_case_2_env_dispatch() {
  print_section("2. Env name -> URL (replaces string switch)");

  static MAYBE_CONSTEXPR frozen_map<std::string_view,
    "dev"_fs, "stg"_fs, "prd"_fs> kEnv{
    "https://dev.example.com",
    "https://stg.example.com",
    "https://example.com"
  };

  for (auto env : std::array<std::string_view, 4>{"dev", "stg", "prd", "local"}) {
    auto const v = kEnv.get(env);
    auto const url = v ? v->get() : std::string_view{"(default: https://localhost)"};
    std::cout << "  " << env << " -> " << url << '\n';
  }
}

// 3. 設定バンドル (バラけた constexpr の集約)
// before: namespace cfg { inline constexpr int kTimeout = 30; ... }
// after : 1 か所に集約、glaze で JSON 上書き可能
void use_case_3_config_bundle() {
  print_section("3. HTTP client config bundle (replaces scattered constexpr)");

  static MAYBE_CONSTEXPR frozen_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs> kHttp{ 30, 5, 2 };

  std::cout << "  timeout = " << kHttp["timeout"] << '\n';
  std::cout << "  retry   = " << kHttp["retry"]   << '\n';
  std::cout << "  backoff = " << kHttp["backoff"] << '\n';

  // one-line get-with-default: replaces optional<reference_wrapper>.value_or()
  std::cout << "  timeout (get_value_or) = " << kHttp.get_value_or("timeout", -1) << '\n';
  std::cout << "  missing (get_value_or) = " << kHttp.get_value_or("missing", -1) << '\n';

  // 未知キーは実行時例外 (typo を開発中に気づける)
  auto const bad_key = std::string_view{"timeut"};
  if (kHttp.contains(bad_key)) {
    std::cout << "  unexpected: '" << bad_key << "' was found\n";
  } else {
    std::cout << "  typo '" << bad_key << "' -> frozen_map does not contain it (caught at contains() check)\n";
  }
}

// 4. to<std::map>() / to<std::array<>>() でレガシー API へ
// before: std::map を毎回組み立てる / キーが固定でも手書き
// after : frozen_map から to<...>() で欲しい形の辞書へ
void use_case_4_to_legacy() {
  print_section("4. to<std::map>() / to<std::array>() for legacy interop");

  static const frozen_map<std::string,
    "host"_fs, "port"_fs, "user"_fs> kServer{
    "localhost", "8080", "admin"
  };

  auto const ordered = kServer.to<std::map<std::string, std::string>>();
  std::cout << "  std::map (lex order):\n";
  for (auto const& [k, v] : ordered) {
    std::cout << "    " << k << " = " << v << '\n';
  }

  auto const slots = kServer.to<std::array<std::pair<std::string_view, std::string>, 3>>();
  std::cout << "  std::array (decl order):\n";
  for (auto const& [k, v] : slots) {
    std::cout << "    " << k << " = " << v << '\n';
  }
}

// 5. glaze 連携: JSON スキーマ駆動の DTO
// before: JSON 入力を受けて自前でキー判定、不要キー除去、型変換
// after : スキーマ = 型。未知キーは無視、スキーマ外のキーは書き出さない
void use_case_5_glaze_schema() {
  print_section("5. glaze JSON schema (frozen_map as DTO)");

#if HAS_GLAZE
  frozen_map<std::string, "id"_fs, "name"_fs, "email"_fs> u{
    std::array<std::string, 3>{"0", "", ""}
  };

  // 未知キー "admin" は読み込み時に無視される
  auto const ec = glz::read_json(u,
    R"({"id":"42","name":"Toge","email":"toge@example.com","admin":true})");
  std::cout << "  read_json  ec=" << (ec ? "error" : "ok")
            << " id=" << u["id"]
            << " name=" << u["name"]
            << " email=" << u["email"] << '\n';

  // スキーマのキーのみが出力される
  std::string out;
  auto const w_ec = glz::write_json(u, out);
  std::cout << "  write_json ec=" << (w_ec ? "error" : "ok")
            << " json=" << out << '\n';
#else
  std::cout << "  (skipped: glaze not available)\n";
#endif
}

// 6. inja テンプレートの変数スキーマ
// テンプレートが必要とする変数集合をコンパイル時に固定する。
// テンプレートに変更が入ったらスキーマ側のチェックで検出。
void use_case_6_inja_schema() {
  print_section("6. inja template variable schema (compile-time check)");

  // テンプレートが必要とする変数のスキーマ (値は inja に流すデフォルト)
  // 値は string_view にして consteval/constexpr 構築を保証する。
  static MAYBE_CONSTEXPR frozen_map<std::string_view,
    "user_name"_fs, "verify_url"_fs, "expires_at"_fs> kEmailVars{
    "anonymous", "https://example.com/verify", "1 hour"
  };

  // 実行時: テンプレートが要求する変数がスキーマに揃っているか確認
  for (auto v : std::array<std::string_view, 4>{"user_name", "verify_url", "expires_at", "phone"}) {
    std::cout << "  template uses " << v << " -> "
              << (kEmailVars.contains(v) ? "ok" : "MISSING in schema") << '\n';
  }

  // set-membership check: all keys present at once (consteval)
  static constexpr bool has_core =
    kEmailVars.contains_all<"user_name"_fs, "verify_url"_fs>();
  static constexpr bool has_all =
    kEmailVars.contains_all<"user_name"_fs, "verify_url"_fs, "expires_at"_fs>();
  std::cout << "  contains_all (user+url): " << has_core << '\n';
  std::cout << "  contains_all (all 3):    " << has_all << '\n';

#if HAS_INJA
  // 実際の inja レンダリング
  constexpr auto tmpl = frozenchars::FrozenString{
    "Hi {{ user_name }}, click {{ verify_url }} by {{ expires_at }}."
  };

  // スキーマを走査して inja のコンテキストを組み立てる
  // (std::string への変換はランタイムの通常コンストラクタでOK)
  auto ctx = frozenchars::inja::inja_object{};
  ctx.reserve(kEmailVars.size());
  // keys_in_declaration_order() returns the non-sorted key span (declaration order)
  for (auto const& k : kEmailVars.keys_in_declaration_order()) {
    ctx.emplace(std::string{k}, frozenchars::inja::inja_value{std::string{kEmailVars[k]}});
  }
  auto const rendered = frozenchars::inja::render<tmpl>(
    frozenchars::inja::inja_value{std::move(ctx)});
  std::cout << "  rendered: " << rendered << '\n';
#else
  std::cout << "  (inja render skipped: FROZENCHARS_ENABLE_INJA not set)\n";
#endif
}

// 7. 拡張子 -> ハンドラ (関数ポインタディスパッチ)
// before: static const std::unordered_map<std::string, Handler> kLoaders = { ... };
// after : キー集合と同じ連続メモリに関数ポインタを並べる
std::string load_json(std::string_view body) { return "JSON(" + std::string{body} + ")"; }
std::string load_csv (std::string_view body) { return "CSV("  + std::string{body} + ")"; }
std::string load_tsv (std::string_view body) { return "TSV("  + std::string{body} + ")"; }

void use_case_7_handler_dispatch() {
  print_section("7. Extension -> handler (function pointer dispatch)");

  using Handler = std::string (*)(std::string_view);
  static MAYBE_CONSTEXPR auto kLoaders = make_frozen_map<Handler,
    "json"_fs, "csv"_fs, "tsv"_fs>(
    std::pair{"json", &load_json},
    std::pair{"csv",  &load_csv},
    std::pair{"tsv",  &load_tsv}
  );

  for (auto ext : std::array<std::string_view, 4>{"json", "csv", "tsv", "xml"}) {
    if (auto h = kLoaders.get(ext); h) {
      std::cout << "  " << ext << " -> " << (*h)("payload") << '\n';
    } else {
      std::cout << "  " << ext << " -> (no loader)\n";
    }
  }
}

// Bonus: operator== / operator!= (value-wise comparison)
void use_case_bonus_equality() {
  std::cout << "\n--- Bonus: operator== / operator!= ---" << std::endl;

  using endpoint_map = frozen_map<std::string_view, "protocol"_fs, "host"_fs, "port"_fs>;
  endpoint_map ep1{std::array<std::string_view, 3>{"https", "example.com", "443"}};
  endpoint_map ep2{std::array<std::string_view, 3>{"https", "example.com", "443"}};
  endpoint_map ep3{std::array<std::string_view, 3>{"http", "example.com", "80"}};
  std::cout << "ep1 == ep2: " << (ep1 == ep2) << '\n';  // 1
  std::cout << "ep1 != ep3: " << (ep1 != ep3) << '\n';  // 1
}

} // namespace

int main() {
  use_case_1_http_status();
  use_case_2_env_dispatch();
  use_case_3_config_bundle();
  use_case_4_to_legacy();
  use_case_5_glaze_schema();
  use_case_6_inja_schema();
  use_case_7_handler_dispatch();
  use_case_bonus_equality();

  std::cout << "\nAll 7 frozen_map use cases demonstrated.\n";
  return 0;
}
