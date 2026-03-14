# Plan Execution State

Last updated: 2026-03-14

## Baseline
- Tests: 1635/1770 passed (92%), 135 failed (unchanged after all Phase 1 work)

## Completed: Phase 0 — Structure Refactor

All 6 slices done.

## Completed: Phase 1 — Output Contract and Public API

All work items done:

- [x] Split include paths into quote / normal / system / after buckets
- [x] Add public API for source-based preprocessing and file name control (`preprocess_source()`)
- [x] Add public API for define/undefine (`define_macro()`, `undefine_macro()`)
- [x] Add initial line marker emission (`# 1 "filename"`)
- [x] Add include enter and include return markers (`# 1 "file" 1` / `# N "file" 2`)
- [x] Move __FILE__, __LINE__, __BASE_FILE__, __COUNTER__ into explicit managed state
- [x] Wire CLI flags (-D, -U, -I, -iquote, -isystem, -idirafter) in c4cll driver
- [x] Side-channel: not implemented as separate containers (deferred — not blocking anything)

## Completed: Phase 2/3 partial — Include/Pragma System

- [x] Support `<...>` angle bracket includes (done with path buckets)
- [x] Implement `#pragma once`
- [x] Make unknown pragmas non-fatal (ignore instead of fallback)

## Completed: Phase 5 partial — Macro Expansion

- [x] Fix variadic macro __VA_ARGS__ spacing (all preprocessor pending features resolved)

## Next Phase: Phase 2 remainder

### Not Started
- Support computed includes after macro expansion
- Implement full include search order
- Add include resolution cache
- Add `#include_next`
- Add include guard optimization
- Preserve symlink-aware path behavior where practical

### Suggested Next Slice
- `#include_next` support (needed for system header chaining)
