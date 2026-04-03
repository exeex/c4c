# Std Vector Success-Path And STL Throughput Follow-On

Status: Complete
Last Updated: 2026-04-02

## Goal

Make the motivating `std::vector` / library-header validation path usable when
parser work succeeds or takes a long time, without reopening the completed
parser-debug observability runbook.

## Why This Idea Exists

The completed parser-debug surface runbook established better failure-local
output, tentative lifecycle tracing, injected-token markers, and narrower CLI
flags. During Step 5 validation:

- `tests/cpp/eastl/eastl_vector_simple.cpp` reached a real parser failure and
  benefited from the new failure-local and tentative output once invoked with
  `-I ref/EASTL/include -I ref/EABase/include/Common`.
- `tests/cpp/std/std_vector_simple.cpp` did not finish `--parse-only` within a
  60s timeout in this environment, so the main remaining obstacle there is not
  missing trace payload.

## Completed Outcome

- Added a repo-local EASTL vector parse wrapper at
  `tests/cpp/eastl/run_eastl_vector_parse_recipe.cmake`.
- Added the standalone build target
  `cmake --build build --target eastl_vector_simple_parse_recipe`.
- Added the focused regression test `cpp_eastl_vector_parse_recipe` to lock the
  include-path recipe plus the committed parser failure location.
- Added the standalone build target
  `cmake --build build --target std_vector_simple_parse_baseline` so the
  bounded `std::vector` timeout remains reproducible after this plan is closed.

## Measured Baseline

- `tests/cpp/std/std_vector_simple.cpp` still times out under a bounded 60s
  `--parse-only` run with no stdout or stderr.
- `tests/cpp/eastl/eastl_vector_simple.cpp` now has a checked recipe using
  `-I ref/EASTL/include -I ref/EABase/include/Common` and still reaches the
  committed parser failure at
  `ref/EASTL/include/EASTL/internal/type_properties.h:35:52`.

## Leftover Issues

- Remaining `std::vector` success-path visibility and throughput work was split
  into `ideas/open/std_vector_success_path_visibility_and_throughput_follow_on.md`.

## Non-Goals

- Do not reopen the completed failure-local / tentative / injected trace slice.
