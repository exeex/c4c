# RV64 Runtime Local Stack Memory Foundation

## Goal

Extend the qemu-backed rv64 runtime path from `local_temp.c` into a small,
semantic local stack memory foundation: local arrays, direct local pointer
deref, and simple local member/offset cases that stay inside frame-slot based
memory.

## Starting Point

Ideas 295 and 296 established:

- rv64 qemu runtime CTest plumbing;
- fallback rejection for BIR/LLVM text before link/run;
- simple branch/runtime scalar execution;
- narrow local-slot support proven by `local_temp.c`;
- 14 passing `backend_rv64_runtime_*` cases.

The next memory step should keep the scope deliberately local. Do not combine
this with global memory, function-call ABI growth, aggregates-by-value, or broad
pointer provenance.

## Candidate Cases

Start from existing `tests/backend/case/` files. Suggested order:

1. `local_array.c`
2. `local_pointer_deref.c`
3. `local_dynamic_member_array.c` only if the first two do not require broad
   pointer/index machinery
4. `packed_local_member_offsets.c` only if the previous cases establish the
   needed frame-slot and offset support cleanly

If a candidate pulls in globals, calls, aggregate ABI, dynamic alloca, or
non-local provenance, stop and open a narrower follow-up instead of stretching
this idea.

## In Scope

- Add rv64 runtime tests using `c4c_add_backend_rv64_runtime_case(...)`.
- Support frame-slot based I32 local loads/stores beyond the single
  `local_temp.c` shape.
- Support simple base-plus-constant-offset addressing when backed by prepared
  memory/access metadata.
- Support named pointer/local values only to the extent needed for selected
  local frame-slot cases.
- Preserve fail-closed behavior for unsupported memory forms.

## Out of Scope

- Global memory and string constants.
- Function-call ABI expansion.
- Aggregate returns or by-value aggregate arguments.
- Dynamic alloca/VLA.
- Pointer provenance beyond local frame-slot base-plus-offset facts.
- Filename-specific or exact-source-shape shortcuts.

## Acceptance Criteria

- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with at least one new local stack memory case beyond `local_temp.c`.
- The rv64 runner still rejects BIR and LLVM fallback text before linking.
- Unsupported local memory forms fail closed instead of emitting partial,
  runnable nonsense.
- Existing RISC-V focused tests remain monotonic:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared prepared-memory or backend routing behavior changes materially.

## Completion Notes

Closed after the active runbook completed Step 5 validation.

Implemented and registered qemu-backed rv64 runtime support for:

- `local_array.c`
- `local_pointer_deref.c`
- `packed_local_member_offsets.c`

The accepted support is based on prepared local frame-slot and
base-plus-constant-offset metadata, including local I32 load/store offsets,
direct local pointer dereference, pointer-width local slot load/store,
frame-slot address materialization, I8 stores, unaligned I32 bytewise
loads/stores, stack-home load results, and simple prepared cast/select
materialization.

`local_dynamic_member_array.c` remains outside this idea's scope because it
crosses a helper call and pointer-parameter member indexing, which belongs to
call/argument handling and non-local pointer provenance work rather than this
local stack memory foundation.

Validation recorded at close:

- Regression guard over canonical rv64 runtime logs passed:
  `test_before.log` had 16 passing tests and `test_after.log` had 17 passing
  tests, with no new failures.
- Step 5 rv64 runtime acceptance passed all 17
  `backend_rv64_runtime_*` cases.
- Step 5 RISC-V focused acceptance preserved the pre-existing
  `backend_riscv_prepared_edge_publication` failure while both
  `backend_codegen_route_riscv64_*` tests passed; the same failure was
  reproduced with this slice stashed.
- Reviewer report `review/rv64_step4_route_review.md` judged the route aligned
  with this idea and not testcase-overfit, with a watch note to keep the
  pointer-index skip limited to prepared frame-slot address materialization.

## Reviewer Reject Signals

- The implementation accepts fallback BIR/LLVM as runtime success.
- The emitter keys behavior to a case filename or exact source text.
- The route silently expands into globals, calls, aggregate ABI, or dynamic
  stack support.
- Existing AArch64/backend route tests are weakened.
- The proof replaces qemu execution with host-native execution.

## Notes For The Agent

- Inspect the current rv64 prepared emitter in
  `src/backend/mir/riscv/codegen/emit.cpp`.
- Prefer prepared memory-access and value-home metadata over rendered text.
- Keep local stack support narrow and explicit; failing closed is acceptable
  when a case exceeds the source idea.
- If every local candidate exposes a different hard problem, split rather than
  forcing one broad patch.
