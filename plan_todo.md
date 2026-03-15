# Plan Execution TODO

Last updated: 2026-03-15

## Baseline
- Tests: 1784/1784 passed (100%), 0 failed

## Completed This Iteration
- [x] Add regression tests for `#include_next` (4 sub-tests: basic, angle, chain, cross-bucket)
- [x] Fix `#include_next` bug: infinite recursion when CurDir entry duplicates -I path
- [x] Add regression tests for include resolution cache (3 sub-tests: basic, dir-qualified, angle)
- [x] Add regression tests for include guard optimization (5 sub-tests: basic, content-before, undef, pragma-once coexist, nested)
- [x] Add CLI integration tests for `-I`, `-isystem`, `-iquote`, `-idirafter` (6 sub-tests including priority and -D interaction)
- [x] Full suite: 1784/1784 (100%), no regressions

## Bug Found & Fixed
- `#include_next` with quoted includes: CurDir entry added to search list caused `starts_with` to match the CurDir entry, setting `start_idx=1`, but the same directory appeared as a normal `-I` path at index 1 → infinite recursion.
- Fix: skip CurDir entry when `is_include_next` is true.

## Next Suggested Work
- `__VA_OPT__` support (C2x feature)
- `#pragma redefine_extname` (Phase 3)
- `__attribute__((visibility("...")))` on individual declarations
- `__has_include_next(...)` proper next-search semantics
