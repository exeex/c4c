# RV64 Object Frame And Stack Helper Cleanup

## Goal

Extract pure RV64 object-route frame sizing, stack offset, register-home lookup, basic stack load/store, and stack adjustment helpers into a narrower frame-helper boundary.

## Why This Exists

The RV64 object route mixes pure frame math with call lowering, memory access, formal-entry homes, and object-function traversal. A small frame-helper cleanup can reduce coupling before local memory, scalar, call, or traversal movement.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
- Extract pure frame sizing, stack-slot offset, register-home lookup, basic stack load/store, and simple stack adjustment helpers.
- Preserve existing prepared stack-layout diagnostics and object-route behavior.
- Keep helper signatures compatible with later local memory and call slices.

## Out Of Scope

- Call-specific byval/sret argument publication, before-return bundles, function traversal, and prepared instruction dispatch.
- Frame-size, alignment, ABI, diagnostic, or unsupported-contract changes.
- RV64 capability repair, gcc_torture expectation changes, unsupported marker changes, or target-side inference.

## Acceptance Criteria

- Pure frame helpers are separated without changing stack-frame size, offset meaning, or formal-entry home semantics.
- The change does not move `prepared_function_to_object_function` or broad function traversal.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_stack_passed_parameter_home_publication|obj_runtime_rv64_large_fixed_frame_slot_access|obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload)'`

## Reviewer Reject Signals

- The diff changes stack-frame size, alignment, formal-entry home meaning, or unsupported diagnostics.
- The slice mixes frame math with call lowering, local memory semantics, or broad traversal movement.
- The implementation adds testcase-shaped stack-slot shortcuts or named-case handling.
- Tests are weakened, unsupported markers are added, or expectations are rewritten to claim progress.
- The same frame/call/memory coupling is retained behind renamed helpers.
