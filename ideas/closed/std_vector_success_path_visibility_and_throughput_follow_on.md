# Std Vector Success-Path Visibility And Throughput Follow-On

Status: Complete
Last Updated: 2026-04-02

## Goal

Address the remaining `tests/cpp/std/std_vector_simple.cpp` validation friction
after the include-path wrapper slice was completed for the motivating STL
workflows.

## Current Baseline

- `cmake --build build --target std_vector_simple_parse_baseline` now provides
  a repo-local reproduction of the current `std::vector` behavior.
- The bounded baseline still times out after 60s with no stdout or stderr when
  parser debug is not requested.
- `cmake --build build --target eastl_vector_simple_parse_recipe` and
  `cpp_eastl_vector_parse_recipe` already cover the separate EASTL include-path
  workflow, so that slice should not be reopened here.

## Concrete Problems

- There is still no high-signal success-path summary for long successful or
  near-successful STL parses.
- `std_vector_simple.cpp` remains above a practical manual investigation window,
  so parser bring-up still lacks a tight edit-run loop on that motivating case.

## Candidate Directions

- Measure where the 60s bounded `std::vector` parse spends time before changing
  parser behavior.
- Add a success-path parser summary only if it materially improves manual
  investigation for this motivating case.
- Prefer the smallest measurement-backed slice over general parser
  optimization.

## Non-Goals

- Do not reopen the completed EASTL include-path wrapper slice.
- Do not broaden into unrelated STL parser failures.

## Completion Notes

- Reproduced the motivating baseline via
  `cmake --build build --target std_vector_simple_parse_baseline`; it still
  times out after 60s as expected.
- Confirmed the original blind spot with bounded direct runs: both
  `timeout 5s build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
  and the same command with `--parser-debug` exited with `124`, which showed
  that parser debug emitted no success-path signal before timeout.
- Added a throttled live parser-debug progress line for long-running parses, so
  `timeout 7s build/c4cll --parse-only --parser-debug tests/cpp/std/std_vector_simple.cpp`
  now emits `[pdebug] progress ...` lines once per second before the timeout.
- Added `cpp_std_vector_parse_debug_progress` to keep that bounded workflow
  covered in CTest without changing the silent default baseline target.

## Leftover Issues

- Full-suite `ctest --test-dir build -j --output-on-failure` on 2026-04-02
  still reported three unrelated failures outside this slice:
  `positive_sema_linux_stage2_repro_03_asm_volatile_c`,
  `backend_lir_adapter_aarch64_tests`, and
  `llvm_gcc_c_torture_src_20080502_1_c`.
