# RV64 Object Call Variadic Prologue And Return Fragment Cleanup

## Goal

Split RV64 object-route call, variadic, prologue/epilogue, and return fragments only after lower-level frame, scalar, and memory helpers have stable APIs.

## Why This Exists

Call, variadic, prologue, and return fragments cross saved-register behavior, sret/byval handling, stack frame setup, before-return moves, local/global address helpers, and object fixups. They are too risky for early movement but need a concrete late cleanup contract.

## In Scope

- Owned files:
  - `src/backend/mir/riscv/codegen/object_emission.cpp`
  - `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_call_emit.hpp`
  - `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
  - `src/backend/mir/riscv/codegen/prepared_frame_emit.hpp`
  - possible new compiled RV64 object helper files if `variadic.cpp`, `prologue.cpp`, or `returns.cpp` are not revived first
- Split object-route call fragments, variadic fragments, prologue/epilogue fragments, and return fragments behind behavior-preserving helper APIs.
- Treat legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, and `returns.cpp` as references unless the idea explicitly creates live compiled ownership.
- Preserve call preservation, variadic helper resource authority, sret/byval behavior, and before-return semantics.

## Out Of Scope

- New call or variadic capability repair, variadic admission changes, preserved-register behavior changes, runtime expectation changes, unsupported marker changes, or target-side inference.
- Moving final object module assembly, public ELF entrypoints, or `prepared_function_to_object_function`.

## Acceptance Criteria

- The movement happens after required frame/scalar/memory helper prerequisites or documents an explicit reviewer-approved waiver.
- Legacy owner files are not used as live destinations unless build ownership is created and proven.
- Validation includes:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|codegen_route_riscv64_byval|dump_riscv64_byval|obj_runtime_rv64_local_arg_call|obj_runtime_rv64_callee_saved_gpr_live_across_call|cli_riscv64_variadic_aggregate_overflow_helper_contract_obj)'`

## Reviewer Reject Signals

- The slice revives legacy owner files without CMake/build ownership.
- The diff changes variadic admission, call-boundary effects, preserved register behavior, sret/byval handling, or runtime expectations.
- The implementation mixes behavior-preserving movement with call/variadic capability repair.
- Unsupported markers, gcc_torture expectations, or runtime expectations are weakened.
- A broad new call helper hides the same prologue/return/variadic/function traversal coupling.
