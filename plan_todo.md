# Plan Execution State

Last updated: 2026-03-13

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

## Next Phase: Phase 1 — Output Contract and Public API

### Not Started
- Split include paths into quote / normal / system / after buckets
- Add public API for source-based preprocessing and file name control
- Add public API for define/undefine
- Add initial line marker emission
- Add include enter and include return markers
- Move __FILE__, __LINE__, __BASE_FILE__, __COUNTER__ into explicit managed state
- Define side-channel containers for pragma and macro-expansion metadata
