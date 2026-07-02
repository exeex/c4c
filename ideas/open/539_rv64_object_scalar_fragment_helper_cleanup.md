# RV64 Object Scalar Fragment Helper Cleanup

## Goal

Extract RV64 object-route scalar arithmetic, casts, compare branches, move-to-register/location, and simple return helpers without absorbing select-edge publication or broad dispatch.

## Why This Exists

Scalar helpers are a large region in `object_emission.cpp` and depend on encoders, stack helpers, local/global address behavior, prepared publication plans, and branch fixups. A scalar-only cleanup should peel core scalar fragments before select/publication movement.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- Extract scalar binary, cast, compare-branch, move-to-register/location, and simple return helpers.
- Preserve compare predicate normalization, branch-label fixup behavior, diagnostics, and emitted bytes.
- Depend on stable encoder and frame helper boundaries where needed.

## Out Of Scope

- Select-edge publication, predecessor publication movement, before-return move bundles, full terminator dispatch, and `fragment_for_prepared_instruction` movement.
- Branch behavior changes, RV64 capability repair, gcc_torture expectation changes, unsupported marker changes, or target-side inference.

## Acceptance Criteria

- Scalar helper ownership is narrower without creating a catch-all scalar monolith.
- Select/publication and broad dispatch remain separate.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'`

## Reviewer Reject Signals

- The slice rewrites select publication, prepared edge movement, or broad instruction dispatch while named scalar-only.
- The diff changes emitted branch behavior, compare predicate normalization, diagnostics, or object bytes.
- Tests are weakened, unsupported markers are added, or expectations are rewritten.
- The implementation adds named-case scalar shortcuts instead of preserving generic prepared scalar behavior.
- The old scalar/select/dispatch coupling survives under a new abstraction name.
