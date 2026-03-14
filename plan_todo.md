# Plan Execution State

Last updated: 2026-03-14

## Baseline
- Tests: 1635/1770 passed (92%), 135 failed

## Completed: Phase 0 — Structure Refactor

All 6 slices done. No regressions (1635/1770 before and after).

- Slice 1: `pp_text.hpp/cpp` — trim_copy, is_ident_start, is_ident_continue, join_continued_lines, strip_comments
- Slice 2: `pp_include.hpp/cpp` — dirname_of, read_file, shell_quote, run_capture, preprocess_external
- Slice 3: `pp_pragma.hpp/cpp` — dispatch_pragma (PragmaResult: Handled/Ignored/Unhandled)
- Slice 4: `pp_macro_def.hpp` + `pp_predefined.hpp/cpp` — MacroDef, MacroTable, init_predefined_macros
- Slice 5: `pp_macro_expand.hpp/cpp` — collect_funclike_args, stringify_arg, find_param_idx, substitute_funclike_body, split_directive
- Slice 6: `pp_cond.hpp/cpp` — resolve_defined_and_intrinsics, ExprValue, IfExprParser

Result: preprocessor.cpp reduced from 1541 to 518 lines (orchestration only).

## In Progress: Phase 1 — Output Contract and Public API

### Completed
- [x] Split include paths into quote / normal / system / after buckets
  - Replaced single `include_paths_` with 4 buckets: `quote_include_paths_`, `normal_include_paths_`, `system_include_paths_`, `after_include_paths_`
  - Added public API: `add_quote_include_path()`, `add_include_path()`, `add_system_include_path()`, `add_after_include_path()`
  - Updated `handle_include()` to support `#include <...>` with GCC-compatible search order
  - Added 7 test cases covering all buckets and priority ordering

- [x] Add public API for define/undefine
  - Added `define_macro(def)`: supports `"FOO"` (=1) and `"FOO=bar"` forms
  - Added `undefine_macro(name)`
  - Added 4 test cases

- [x] Add initial line marker emission
  - Emit `# 1 "filename"` at start of preprocessed output
  - Added test for initial marker

- [x] Add include enter and include return markers
  - Emit `# 1 "included_file" 1` on include enter
  - Emit `# N "parent_file" 2` on include return
  - Added test for enter/return markers

- [x] Move __FILE__, __LINE__, __BASE_FILE__, __COUNTER__ into explicit managed state
  - __BASE_FILE__: set once in preprocess_file(), constant across includes
  - __COUNTER__: auto-incrementing counter (handled specially in expand_text)
  - __FILE__/__LINE__: continue using macro table updates in preprocess_text() loop
  - Added tests for __BASE_FILE__ and __COUNTER__

- No regressions throughout (1635/1770 before and after all slices)

### Not Started
- Add public API for source-based preprocessing and file name control
- Define side-channel containers for pragma and macro-expansion metadata

### Next Slice
- Add public API for source-based preprocessing (preprocess from string with configurable filename)
