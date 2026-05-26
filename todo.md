Status: Active
Source Idea Path: ideas/open/11_aarch64_calls_file_consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Simplify calls.hpp

# Current Packet

## Just Finished

Step 3 - Simplify `calls.hpp` completed after the target operand adapter
merge. Removed stale public declarations from `calls.hpp` for helpers proven to
have no external users, removed the now-unused `<string_view>` include, and
kept the active target-emission APIs that still cross translation-unit
boundaries.

AST and search checks used:

- `c4c-clang-tool function-signatures` inventoried the `calls.hpp` declaration
  surface.
- `c4c-clang-tool-ccdb list-symbols` inspected the relevant calls translation
  units before cleanup.
- `rg` proved removed declarations had no external clients beyond their owning
  implementation files or existing narrower `dispatch_diagnostics.hpp`.
- `c4c-clang-tool-ccdb find-definition` confirmed `align_to` and
  `prepared_indirect_byval_extent_bytes` now resolve with anonymous-namespace
  linkage in their implementation files.

Changed files:

- `src/backend/mir/aarch64/codegen/calls_common.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls.hpp`
- `todo.md`
- `test_after.log`

## Suggested Next

Supervisor should review/commit this completed Step 3 header simplification
slice. The next coherent packet is Step 4 stale calls build-entry/file cleanup,
but only for files or build entries that are now provably retired.

## Watchouts

- This packet changed declaration visibility only; no semantic lowering,
  dispatch bridge behavior, ABI classification, or test expectations changed.
- `calls.hpp` still intentionally exposes active cross-TU target emission APIs,
  including call-plan lookup/lowering, move lowering, target operand adapters,
  selected call argument source conversion, and byval lane helpers used by
  `calls_moves.cpp`.
- `append_call_diagnostic` remains available through the existing narrower
  `dispatch_diagnostics.hpp` owner rather than `calls.hpp`.
- Leave unrelated transient `review/` artifacts untouched.
- Do not fold `calls_dispatch_bridge.*`, `dispatch.cpp`, or broad
  `calls_moves.cpp` behavior into this consolidation route.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract)$'`

Result: build succeeded; 3/3 focused tests passed.
