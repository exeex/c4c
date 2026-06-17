# RV64 Prepared Emitter Decomposition

## Goal

Reshape the current rv64 prepared emitter so new capabilities are implemented
by functional owner files instead of continuing to grow
`src/backend/mir/riscv/codegen/emit.cpp`.

This is a behavior-preserving decomposition idea. It should not add new rv64
runtime cases or broaden backend capability while the split is in progress.

## Why This Exists

Ideas 295 through 300 successfully brought up qemu-backed rv64 runtime
coverage from zero to the current foundation set. The route is real:

- `c4cll --codegen asm --target riscv64-linux-gnu`;
- fallback rejection for BIR/LLVM text;
- clang rv64 cross compile/link;
- `qemu-riscv64` execution.

However, the implementation has concentrated in
`src/backend/mir/riscv/codegen/emit.cpp`, which now owns too many unrelated
responsibilities:

- prepared function/block traversal;
- prologue/epilogue and `ra` spill placement;
- scalar binary/cast/select/return emission;
- direct scalar call lowering;
- local frame-slot and pointer-parameter memory lowering;
- frame-slot address materialization helpers;
- edge publication move adaptation;
- final public prepared-module text emission.

The AArch64 backend gives the healthier model: a small public entry, an
internal module coordinator, dispatch/traversal, and family-specific owners
such as `calls`, `memory`, `alu`, `comparison`, `returns`, and
value-materialization helpers. RISC-V should move toward that shape before
more capability is added.

## AArch64-Inspired Target Layout

Use the current AArch64 codegen layout as the reference, but keep the first
rv64 split smaller and behavior-preserving.

Initial proposed RISC-V owner surfaces:

- `prepared_module_emit.{hpp,cpp}`
  - public prepared-module text entry currently implemented by
    `emit_prepared_module(...)`;
  - orchestration only, not feature-family lowering.
- `prepared_function_emit.{hpp,cpp}`
  - function/block traversal;
  - label spelling;
  - prologue/epilogue coordination;
  - dispatch order over BIR instructions and terminators.
- `prepared_scalar_emit.{hpp,cpp}`
  - scalar immediates and value movement;
  - integer binary ops;
  - casts;
  - select materialization;
  - scalar returns.
- `prepared_call_emit.{hpp,cpp}`
  - direct scalar call lowering;
  - prepared call-plan validation;
  - argument/result register moves;
  - fail-closed checks for unsupported call forms.
- `prepared_local_memory_emit.{hpp,cpp}`
  - local frame-slot loads/stores;
  - pointer-width local slot load/store;
  - local pointer/member-index memory accesses;
  - unaligned I32 bytewise load/store helpers.
- `prepared_frame_emit.{hpp,cpp}` or a small support surface
  - stack-frame sizing/alignment;
  - `ra` spill slot selection;
  - frame-slot stack-offset helpers.
- `prepared_edge_publication_emit.{hpp,cpp}`
  - existing edge publication move adapter and helpers.
- `emit.cpp`
  - retain only compatibility wrappers, legacy direct-LIR tiny-add helper if
    still needed, and public namespace glue that cannot yet move.

Do not recreate a central "records" pile. Shared structs/helpers should live
near the owner that consumes them unless multiple owners actually need them.

## In Scope

- Move code out of `src/backend/mir/riscv/codegen/emit.cpp` into functional
  owner files.
- Update build metadata so the new `.cpp` files compile into `c4c_backend`.
- Preserve all public entrypoints and test behavior.
- Keep helper names boring and ownership-oriented.
- Add only minimal headers needed for cross-owner calls.
- Keep fail-closed behavior unchanged for unsupported rv64 forms.

## Out of Scope

- Adding new rv64 runtime cases.
- Expanding rv64 support for globals, indirect calls, varargs, stack arguments,
  aggregate ABI, dynamic stack, or object emission.
- Rewriting RISC-V around AArch64 machine-node infrastructure in this first
  split.
- Changing the qemu runner contract.
- Deleting the existing edge-publication path or changing its semantics.

## Suggested Execution Order

1. Inventory `emit.cpp` by functional clusters and mark exact move targets.
2. Extract frame/label/basic support helpers that other owners need.
3. Extract scalar return/binary/cast/select logic and prove rv64 runtime stays
   green.
4. Extract local memory/pointer memory logic and prove rv64 runtime stays
   green.
5. Extract direct scalar call logic and prove rv64 runtime stays green.
6. Extract edge-publication adapter or leave it as the last remaining large
   cluster if moving it would obscure behavior.
7. Trim `emit.cpp` to orchestration/public glue and run broader validation.

Each step should be mechanically reviewable. If a step discovers that a helper
requires semantic redesign, stop and open a new capability idea instead of
changing behavior inside this decomposition.

## Acceptance Criteria

- `emit.cpp` no longer owns the majority of scalar, call, local-memory, and
  frame helper implementation.
- New files follow feature ownership rather than testcase families.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes after each meaningful extraction.
- Existing RISC-V focused tests remain monotonic:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- Run `ctest --test-dir build -R '^backend_' --output-on-failure` before
  closure because the split changes backend build/link surfaces.
- No new runtime case or backend capability is added under this idea.

## Reviewer Reject Signals

- A split commit also changes emitted assembly semantics without a separate
  repair explanation.
- New files are organized by testcase name or recent idea number instead of
  backend responsibility.
- `emit.cpp` remains the place where future scalar/call/memory capability must
  be added.
- The route weakens qemu runtime tests or accepts BIR/LLVM fallback.
- The route uses AArch64 as a copy-paste source rather than as a layout model.

## Notes For The Agent

- Read `src/backend/mir/aarch64/codegen/README.md` before planning the split.
  The relevant lesson is the route shape, not exact code reuse.
- Keep the first pass pragmatic. RISC-V does not need the full AArch64
  machine-node architecture before it can stop piling features into `emit.cpp`.
- Prefer one or two extraction commits with green rv64 runtime proof over a
  giant all-at-once file move.
