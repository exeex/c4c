Status: Active
Source Idea: ideas/open/15_cpp_external_eastl_suite_bootstrap.md
Source Plan: plan.md

# Current Packet

Bootstrap `tests/cpp/external/eastl` with one standalone frontend-smoke case,
then prove the new suite path through configure/build/ctest.

## Just Finished

- Activated the new EASTL external-suite bootstrap idea into `plan.md` and
  this `todo.md`.
- Defined the initial target shape:
  `tests/cpp/external/eastl` with allowlist + runner + provenance notes.
- Wired the suite into the main test CMake and proved the first passing case:
  `eastl_cpp_external_piecewise_construct_frontend_basic_cpp`.

## Suggested Next

- Add a second EASTL-derived case, likely from tuple, utility, memory, or
  vector coverage, using the same standalone extracted-case pattern.
- Decide whether the next expansion should stay frontend-only for a few cases or
  deliberately target the first runtime-capable external EASTL case.
- Decide whether follow-on should also create a dedicated submodule repository
  for `tests/cpp/external/eastl`, or keep bootstrapping in-tree until a small
  curated bundle exists.

## Watchouts

- Do not move the whole upstream `ref/EASTL/test` harness under
  `tests/cpp/external/eastl`.
- Keep shared include paths injected by the runner instead of hardcoding them
  independently per case.
- Prefer semantic standalone cases over harness-heavy imports.
- Treat the blocked `type_traits` runtime path as follow-on compiler work, not
  as a reason to revert the external-suite bootstrap.

## Proof

- `cmake -S . -B build`
- `cmake --build build --target c4cll -j8`
- `ctest --test-dir build -R '^eastl_cpp_external_' --output-on-failure`
