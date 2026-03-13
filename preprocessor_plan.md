# Preprocessor Implement Plan

## 目標

整理目前 [`src/frontend/preprocessor/preprocessor.cpp`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp) 與參考設計文件 [`ref/claudes-c-compiler/src/frontend/preprocessor/README.md`](/workspaces/c4c/ref/claudes-c-compiler/src/frontend/preprocessor/README.md) 的差距，並制定一個可逐步落地的實作計劃。

## 目前已具備的能力

依照目前 C++ 版本實作，已經有以下基礎能力：

- Translation phases 的一部分：
  - line splicing: `join_continued_lines()`，見 [`preprocessor.cpp:1016`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1016)
  - comment stripping: `strip_comments()`，見 [`preprocessor.cpp:1031`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1031)
- 基本 directive dispatch：
  - `#define` / `#undef`
  - `#if` / `#ifdef` / `#ifndef` / `#elif` / `#else` / `#endif`
  - `#include`（僅支援 `"..."` 相對目前檔案）
  - `#error` / `#warning`
  - `#line`
  - 見 [`preprocessor.cpp:1074`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1074)
- Macro expansion：
  - object-like macro
  - function-like macro
  - `#` stringify
  - `##` token paste
  - variadic macro 與 `__VA_ARGS__`
  - self-recursion 防護（disabled set）
  - 見 [`preprocessor.cpp:1350`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1350)
- `#if` expression evaluator：
  - `defined`
  - `__has_builtin` / `__has_attribute` / `__has_feature` / `__has_extension` / `__has_include` / `__has_include_next`
    目前全部只回傳 `0`
  - recursive-descent integer expression parser
  - 見 [`preprocessor.cpp:296`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L296) 與 [`preprocessor.cpp:1315`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1315)
- 基本 diagnostics side channel：
  - `errors()`
  - `warnings()`
  - 見 [`preprocessor.hpp:25`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.hpp#L25)
- 基本預定義 macros：
  - `__STDC__`, `__STDC_VERSION__`
  - 一批 type/limit/float/byte-order macros
  - 見 [`preprocessor.cpp:853`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L853)

## 與 README 的主要差距

以下差距以「README 明確宣稱有，但目前 C++ 版本沒有或只做部分」為準。

### 1. Public API 與狀態管理差距

README 描述的 preprocessor 是可配置、可查詢 side channel、可切 target 的元件；目前 C++ 版本 public API 幾乎只有 `preprocess_file()` 與 diagnostics getter。

目前缺少：

- `preprocess(source)` / `preprocess_force_include(...)`
- `set_filename()`
- `define_macro()` / `undefine_macro()` 的 public driver API
- `add_include_path()` / `add_quote_include_path()` / `add_system_include_path()` / `add_after_include_path()`
- `set_target()` 與各種 target-related toggles
- `dump_defines()`
- `take_macro_expansion_info()`
- `weak_pragmas` / `redefine_extname_pragmas`
- `__BASE_FILE__`、`__COUNTER__` 等特殊 builtins 的狀態管理

現況證據：

- header 只有極少 public interface，見 [`preprocessor.hpp:18`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.hpp#L18)
- state 只有單一 `include_paths_`，沒有區分 quote/system/after paths，見 [`preprocessor.hpp:86`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.hpp#L86)

### 2. Output contract 與 line marker 差距

README 的輸出契約包含：

- 初始 line marker `# 1 "filename"`
- include 進出時的 GCC-compatible line markers
- pragma synthetic tokens
- macro expansion side-channel

目前 C++ 版本：

- 只在 `#line` directive 時 emit marker
- `#include` 直接展開 child text，沒有 include enter/return markers
- `preprocess_external()` 用 `-P`，也會刻意移除 line markers

現況證據：

- `#line` 才會 emit marker，見 [`preprocessor.cpp:1137`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1137)
- `handle_include()` 直接 `return preprocess_text(...)`，見 [`preprocessor.cpp:1461`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1461)
- external fallback 命令使用 `-P`，見 [`preprocessor.cpp:1538`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1538)

### 3. Include 系統差距

README 的 include 支援範圍很大；目前只支援最小版本。

目前缺少：

- `<...>` system include
- computed include（先 macro expand 再解析）
- `#include_next`
- 完整搜尋順序：current dir / base file dir / `-iquote` / `-I` / `-isystem` / default system / `-idirafter`
- include resolve cache
- include guard optimization
- `#pragma once`
- builtin header injection
- standard headers fallback declarations
- symlink-preserving absolute path behavior

現況證據：

- `handle_include()` 明寫 TODO，只支援 `"..."`，見 [`preprocessor.cpp:1472`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1472)

### 4. Pragma 支援差距

README 將 pragma 視為重要功能；目前 C++ 版本尚未實作，遇到 `#pragma` 直接標記 external fallback。

目前缺少：

- `#pragma once`
- `#pragma pack(...)`
- `#pragma push_macro` / `#pragma pop_macro`
- `#pragma weak`
- `#pragma redefine_extname`
- `#pragma GCC visibility push/pop`
- 忽略未知 pragma 的一致策略

現況證據：

- 見 [`preprocessor.cpp:1134`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1134)

### 5. `#if` expression 與 intrinsic 差距

目前 parser 已有雛形，但和 README 仍有功能差距與正確性風險。

目前缺少或不完整：

- `__has_include` / `__has_include_next` 真正串接 include search
- builtin/attribute tables，讓 `__has_builtin` / `__has_attribute` 不再永遠回 `0`
- `replace_remaining_idents_with_zero` 的明確獨立階段
- 更完整的 literal 處理：
  - 多字元 character literal
  - 更完整 escape sequences
  - 更完整 suffix handling
- parser 失敗時目前會 fallback 到外部 preprocessor，而不是給 deterministic internal diagnostic

現況證據：

- intrinsic TODO 見 [`preprocessor.cpp:354`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L354)
- `evaluate_if_expr()` 失敗會設 `needs_external_fallback_ = true`，見 [`preprocessor.cpp:1330`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1330)

### 6. Macro expansion 行為差距

目前 macro expansion 已經比其他部分完整，但仍未達 README 描述的成熟度。

目前缺少或未明確保證：

- GNU named variadic macro：`args...`
- anti-paste guard，避免 paste 後不小心形成 `//`、`++` 等新 token
- blue-paint / paste-protection sentinel 的明確模型
- `_Pragma("...")` operator 的處理
- reusable expansion state，避免 per-line 配置 `unordered_set`
- macro expansion info side-channel
- 多行 function-like invocation 累積機制

現況證據：

- `MacroDef` 沒有 `has_named_variadic`，見 [`preprocessor.hpp:30`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.hpp#L30)
- `expand_line()` 每次都從 `{}` 開始，沒有 reusable state，見 [`preprocessor.cpp:1457`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1457)
- 主迴圈逐行處理，沒有 pending line accumulation，見 [`preprocessor.cpp:957`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L957)

### 7. 預定義 macro 與 target/config 差距

README 把 target、ABI、PIC、opt level、SIMD、strict ANSI 都納入 preprocessor；目前是固定一組 LP64/arm64-ish 宏，無 public configuration。

目前缺少：

- `__STDC_HOSTED__`
- `__GNUC__`, `__GNUC_MINOR__`, `__VERSION__`
- platform/arch macros：`__linux__`, `__unix__`, `__ELF__`, `__x86_64__`, `__aarch64__`, `__riscv` 等
- `__DATE__`, `__TIME__`
- atomics / SIMD / feature-test / width macros
- `set_target()` / `set_pic()` / `set_optimize()` / `set_strict_ansi()` 等

### 8. Builtin header substitute / fallback macros 差距

README 提到 `<limits.h>`, `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`, `<stdatomic.h>`, `<float.h>`, `<inttypes.h>` 等可由 preprocessor 直接補宏。

目前缺少：

- header-triggered macro injection
- `INT64_C(x)` / `UINT32_C(x)` 這類 function-like builtin macros
- header-not-found 時的 fallback declaration injection

### 9. 模組化結構差距

README 的 reference implementation 已切成：

- `pipeline`
- `macro_defs`
- `conditionals`
- `expr_eval`
- `includes`
- `pragmas`
- `predefined_macros`
- `builtin_macros`
- `text_processing`
- `utils`

目前 C++ 版本幾乎全部塞在單一 [`preprocessor.cpp`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp) 中，後續功能若直接往內堆，維護成本會很高。

### 10. external fallback 依賴過重

README 代表的是「internal preprocessor 本身就應該可用」；目前 C++ 版本仍大量依賴 external fallback 來兜底：

- pragma
- unknown directive
- 非 quoted include
- `#if` parser failure

這讓行為依賴本機工具鏈，且輸出契約會和 internal path 不一致。

現況證據：

- `needs_external_fallback_` 在多處被設為 `true`，見 [`preprocessor.cpp:943`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L943) 與 [`preprocessor.cpp:1134`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp#L1134)

## 建議的實作原則

### 原則 1：先把 external fallback 從「正常流程」降為「最後保險」

優先補齊會頻繁觸發 fallback 的路徑：

- `<...>` / computed include
- pragma policy
- `#if` parser robustness

否則後續功能再多，實際跑大型 codebase 時仍會落回系統 `cpp/cc -E`。

### 原則 2：先拆結構，再補功能

目前檔案超過 1500 行，若直接在單檔追加 include/pragmas/builtin header，風險高。建議至少先切出：

- `text_processing`
- `conditionals`
- `includes`
- `pragmas`
- `predefined_macros`

即使第一版仍維持單一 class，也應先把 helper 移到對應 translation unit。

### 原則 3：先完成「輸出契約」，再擴張 feature matrix

對 frontend 來說，更重要的是：

- line markers 一致
- include 邊界一致
- diagnostics 一致
- macro expansion side-channel 可依賴

這些比先補很多冷門 predefined macros 更值得優先做。

## 分階段實作計劃

### Phase 0: 重構骨架

目標：讓後續實作不再持續膨脹單檔。

工作項目：

1. 將 [`preprocessor.cpp`](/workspaces/c4c/src/frontend/preprocessor/preprocessor.cpp) 拆為多個 `.cpp` / `.hpp`：
   - `text_processing.*`
   - `conditionals.*`
   - `includes.*`
   - `pragmas.*`
   - `predefined_macros.*`
   - `macro_expansion.*`
2. 保留 `Preprocessor` 作為 façade，減少 driver 端改動。
3. 將 `MacroDef`、`ConditionalFrame`、include/path state 移到獨立型別。
4. 為每個模組補最小單元測試入口。

完成標準：

- 行為不變
- 現有測試全過
- `preprocessor.cpp` 降到只剩 pipeline orchestration

### Phase 1: 補齊輸出契約與 driver API

目標：讓 internal preprocessor 成為穩定、可配置的元件。

工作項目：

1. 擴充 public API：
   - `preprocess(source, file)`
   - `define_macro`
   - `undefine_macro`
   - `add_include_path`
   - `add_quote_include_path`
   - `add_system_include_path`
   - `add_after_include_path`
   - `set_filename`
   - `dump_defines`
2. 加入 line marker policy：
   - 初始 marker
   - include enter marker
   - include return marker
3. 將 `__FILE__`、`__LINE__`、`__BASE_FILE__`、`__COUNTER__` 整理成 dedicated state。
4. 定義 side-channel 資料結構：
   - `weak_pragmas`
   - `redefine_extname_pragmas`
   - `macro_expansion_info`

完成標準：

- 不需要 external fallback 也能輸出穩定 line markers
- include 進出位置可被 lexer 正確追蹤

### Phase 2: include system 完整化

目標：消除目前 include 是最大缺口的問題。

工作項目：

1. 支援 `<...>` include。
2. 支援 computed include：
   - 先 macro expand
   - path normalization
3. 建立完整搜尋路徑：
   - current file dir
   - base file dir
   - quote include paths
   - normal include paths
   - system include paths
   - default system include paths
   - after include paths
4. 實作 include resolution cache。
5. 支援 `#include_next`。
6. 實作 `#pragma once` 與 include guard optimization。
7. 保留 symlink-preserving absolute path 行為。

完成標準：

- 移除 `handle_include()` 對非 quoted include 的 fallback
- `__has_include` / `__has_include_next` 可直接重用同一套 resolver

### Phase 3: pragma 系統

目標：移除 pragma 導致的 external fallback。

工作項目：

1. 實作 `handle_pragma()` dispatcher。
2. 支援：
   - `once`
   - `pack(...)`
   - `push_macro`
   - `pop_macro`
   - `weak`
   - `redefine_extname`
   - `GCC visibility push/pop`
3. 定義 synthetic token 輸出格式。
4. 未識別 pragma 改為 ignore 或 warning，不再 fallback。

完成標準：

- `process_directive()` 不再因 `#pragma` 進 external fallback

### Phase 4: `#if` expression 與 intrinsic 補強

目標：使條件編譯路徑足以覆蓋大型 header。

工作項目：

1. 將 expression preprocessing 分為清楚的 stages：
   - resolve `defined` / `__has_*`
   - expand macros
   - replace remaining idents with zero
   - evaluate const expr
2. 讓 `__has_include` / `__has_include_next` 真正查 include resolver。
3. 建立 builtin/attribute feature tables。
4. 補齊 literal parsing：
   - escape sequences
   - multi-char char literals
   - suffix parsing
5. 將 parse failure 轉為 internal diagnostic 或 deterministic false-path，不再直接 fallback。

完成標準：

- 常見 libc / compiler headers 的 `#if` 表達式可穩定通過

### Phase 5: macro expansion 完整度補強

目標：補齊目前 macro engine 的語義缺口。

工作項目：

1. 支援 GNU named variadic macros：`args...`
2. 加入 anti-paste guard
3. 加入 `_Pragma("...")` operator
4. 設計 reusable expansion state，減少 `unordered_set` 重建
5. 補 `macro_expansion_info`
6. 支援多行 macro invocation accumulation

完成標準：

- 巨集行為更接近 GCC/Clang
- 多行 function-like invocation 不需依賴外部 preprocess

### Phase 6: predefined macros 與 target 設定

目標：讓 preprocessor 不再是假設固定單一 host/target。

工作項目：

1. 把 predefined macros 集中到專門模組。
2. 增加 target families：
   - `x86_64`
   - `aarch64`
   - `riscv64`
   - `i686`
   - `i386`
3. 增加 config toggles：
   - PIC
   - optimize
   - GNU89 inline
   - SIMD flags
   - strict ANSI
4. 補齊常用 compatibility macros：
   - `__GNUC__`, `__GNUC_MINOR__`, `__GNUC_PATCHLEVEL__`
   - `__VERSION__`
   - `__STDC_HOSTED__`
   - `__DATE__`, `__TIME__`

完成標準：

- 常見第三方 headers 不再因缺少 compatibility macros 而走錯分支

### Phase 7: builtin header injection / fallback declarations

目標：在缺少完整 sysroot 或系統 header 時仍可編譯更多專案。

工作項目：

1. 實作 well-known header 的 builtin macro injection：
   - `limits.h`
   - `stdint.h`
   - `stddef.h`
   - `stdbool.h`
   - `stdatomic.h`
   - `float.h`
   - `inttypes.h`
2. 補 function-like builtin macros：
   - `INT64_C(x)`
   - `UINT32_C(x)` 等
3. 實作 minimal fallback declarations：
   - `stdio.h`
   - `errno.h`
   - `complex.h`

完成標準：

- 無 sysroot 場景下仍能通過更多前端測試

## 建議優先順序

若只看投資報酬率，建議順序如下：

1. Phase 0
2. Phase 1
3. Phase 2
4. Phase 3
5. Phase 4
6. Phase 5
7. Phase 6
8. Phase 7

原因：

- 目前最容易把流程打回 external fallback 的是 include、pragma、`#if`
- target macros 與 builtin headers 雖重要，但在 core pipeline 穩定前不適合先做

## 每階段的測試策略

### 單元測試

- macro expansion：
  - object-like
  - function-like
  - nested expansion
  - stringify
  - token paste
  - variadic
  - empty variadic with `, ## __VA_ARGS__`
- conditionals：
  - `defined`
  - nested `#if`
  - inactive branch preservation
  - arithmetic / bitwise / ternary expressions
- includes：
  - quoted
  - angle-bracket
  - include search order
  - include guard skip
  - `#pragma once`
  - `#include_next`
- pragmas：
  - push/pop macro
  - pack
  - weak
  - visibility

### 黃金輸出測試

對相同輸入比對：

- expanded text
- line markers
- diagnostics
- pragma side channels

### 對照測試

用 `cc -E` 或 `cpp` 當 oracle，比較：

- macro expansion 結果
- include/search behavior
- `#if` 分支選擇

這類測試應只作為 comparison harness，不應再作為 runtime fallback 的主要依賴。

## 建議先做的第一個 patch

如果下一步要真的開始 implement，我建議第一個 patch 不是直接補 feature，而是先做這三件事：

1. 把 include path state 從單一 `include_paths_` 擴成四類：
   - quote
   - normal
   - system
   - after
2. 補齊 line marker infrastructure：
   - initial marker
   - include enter/return marker
3. 把 pragma handling 抽成獨立函式，即使第一版先只實作 `#pragma once`

這個切入點的理由是：

- 能立刻降低 external fallback 觸發率
- 能為 `__has_include` 與 `#include_next` 鋪路
- 不需要一開始就把整個 macro engine 重寫

## 結論

目前 C++ preprocessor 不是「缺少一兩個 directive」，而是：

- pipeline 骨架已存在
- macro engine 有基本可用性
- 但 include / pragma / public API / output contract 仍與 README 描述有明顯落差

因此建議路線不是零散補洞，而是：

1. 先模組化
2. 先補輸出契約與 include 系統
3. 再收斂 pragma 與 `#if`
4. 最後補 target macros 與 builtin header substitute
