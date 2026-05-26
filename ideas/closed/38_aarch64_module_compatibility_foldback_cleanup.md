# AArch64 Module Compatibility Fold-Back Cleanup

## Goal

Fold legacy AArch64 compatibility projection construction into the module
compile boundary that still needs it.

## Why This Exists

The Step 4 caller audit classified `compatibility_projection.*` as
mechanical fold-back. The helpers build legacy flat projection records at the
module build boundary, while terminal assembly authority is the MIR stream
rather than `FunctionRecord::machine_nodes`. This should be localized under
module compilation instead of living as a separate AArch64 owner family.

## In Scope

- Fold these helper files into the module compilation owner or a
  module-private compatibility section:
  - `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
  - `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- Preserve `module_compile.cpp` / `module_compile.hpp` as the AArch64 module
  build boundary.
- Keep compatibility projection construction limited to legacy consumers that
  still require it.
- Update includes and build metadata only as needed for the fold-back.
- Prefer deleting obsolete projection plumbing when caller evidence proves it
  is no longer needed.

## Out Of Scope

- Making `FunctionRecord::machine_nodes` a new semantic assembly authority.
- Rewriting final assembly emission, machine-module printing, or MIR stream
  ownership.
- Folding calls, dispatch, memory, comparison, or prologue helpers.
- Broad module representation rewrites unrelated to compatibility projection
  construction.

## Acceptance Criteria

- Compatibility projection construction no longer lives in standalone
  AArch64 `compatibility_projection.*` owner files.
- Remaining projection construction is module-private and clearly marked as
  compatibility support, or removed if unused.
- Terminal assembly behavior remains driven by the MIR stream.
- Validation covers AArch64 module compilation and any legacy consumer that
  still reads projection records.

## Reviewer Reject Signals

- The patch reestablishes `FunctionRecord::machine_nodes` as terminal
  assembly authority or routes new behavior through legacy projection records.
- The patch only renames `compatibility_projection.*` helpers while preserving
  the same standalone owner boundary.
- The patch changes final assembly printing, MIR stream ownership, or
  unrelated module representation behavior in this mechanical slice.
- The patch rewrites expectations or removes compatibility coverage instead
  of preserving or deleting projection behavior based on caller evidence.
- The patch mixes module compatibility cleanup with calls, dispatch, memory,
  comparison, or prologue fold-back.

## Closure Note

Closed on 2026-05-26.

The scoped fold-back is complete:

- `compatibility_projection.cpp` / `compatibility_projection.hpp` were deleted
  and removed from build metadata.
- `derive_compatibility_function_records` and
  `derive_compatibility_projection` moved into the anonymous namespace in
  `module_compile.cpp`.
- `module_compile.cpp` no longer includes `compatibility_projection.hpp`.
- `backend_aarch64_signature_metadata_test.cpp` was updated only for
  deleted-file metadata fallout.
- `built_module.functions` and `built_module.compatibility` remain populated as
  before, preserving legacy projection behavior for current readers.
- Live source, test, and build paths no longer reference the deleted
  `compatibility_projection.cpp` / `compatibility_projection.hpp` files.
- Remaining compatibility wording is deliberate module compatibility semantics,
  not standalone owner state.
- Terminal assembly behavior remains driven by the MIR stream; the fold-back
  did not make `FunctionRecord::machine_nodes` an assembly authority and did
  not change final assembly printing or MIR stream ownership.

Closure evidence:

- Focused module/projection proof passed 6/6:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_signature_metadata|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_scalar_alu_records)$' | tee test_after.log`
- Broader backend bucket passed 163/163.
- Matching backend before/after regression guard passed with 163/163 tests
  before and 163/163 tests after.
