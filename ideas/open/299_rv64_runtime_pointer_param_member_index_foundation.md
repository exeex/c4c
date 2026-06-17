# RV64 Runtime Pointer Parameter Member Index Foundation

## Goal

Extend the qemu-backed rv64 runtime route to cover the next local-memory
frontier: a direct scalar helper call that receives a pointer to a local
aggregate and performs simple member-array indexing through that pointer.

## Starting Point

Ideas 295 through 298 established:

- rv64 qemu runtime CTest plumbing with BIR/LLVM fallback rejection;
- immediate branch, scalar return, scalar add/sub, and `local_temp.c`;
- local stack memory cases including `local_array.c`,
  `local_pointer_deref.c`, and `packed_local_member_offsets.c`;
- bounded direct scalar register calls including `two_arg_helper.c` and
  `local_arg_call.c`;
- 19 passing `backend_rv64_runtime_*` cases.

Idea 297 deliberately left `local_dynamic_member_array.c` out of scope because
it combines helper-call argument passing with pointer-parameter member indexing.
This idea owns that seam explicitly.

## Candidate Case

Primary case:

- `tests/backend/case/local_dynamic_member_array.c`

This case should return `11`:

- `main` creates a local `struct Pair { int xs[3]; }`;
- passes `&p` and index `2` to `get_at`;
- `get_at` returns `p->xs[i]`.

## In Scope

- Register `local_dynamic_member_array.c` with
  `c4c_add_backend_rv64_runtime_case(...)`.
- Support the prepared metadata path needed for:
  - passing a local aggregate address as a scalar pointer argument;
  - receiving that pointer parameter in the callee;
  - computing simple member-array element addresses from pointer parameter plus
    scalar index;
  - loading an I32 element from that computed address;
  - returning the scalar result under qemu.
- Preserve fail-closed behavior for unsupported pointer/member forms.
- Use prepared value homes, memory accesses, call plans, and address
  materialization metadata as the source of truth.

## Out of Scope

- Global memory and string constants.
- Indirect calls and function pointer calls.
- Varargs.
- Stack-passed arguments.
- Aggregate/byval/sret ABI.
- Dynamic alloca/VLA.
- General pointer provenance beyond this pointer-parameter local aggregate
  access path.
- Filename-specific or exact-source-shape shortcuts.

## Acceptance Criteria

- `backend_rv64_runtime_local_dynamic_member_array` executes under
  `qemu-riscv64` with expected exit code `11`.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the new case included.
- The rv64 runner still rejects BIR and LLVM fallback before linking.
- Unsupported pointer/member/index forms fail closed instead of emitting partial
  runnable assembly.
- Existing RISC-V focused tests remain monotonic:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- Escalate to `ctest --test-dir build -R '^backend_' --output-on-failure` if
  shared prepared-memory, call-plan, or backend routing behavior changes
  materially.

## Reviewer Reject Signals

- The implementation recognizes `local_dynamic_member_array.c`, `get_at`, or
  the exact source shape instead of lowering from prepared metadata.
- The route expands into globals, indirect calls, varargs, stack args, aggregate
  ABI, or dynamic stack support without a new idea.
- The runtime proof skips qemu.
- The runner accepts BIR/LLVM fallback text.
- Existing backend tests are weakened or expectation-rewritten to hide a pointer
  member indexing failure.

## Notes For The Agent

- This is intentionally the join point between ideas 297 and 298; keep the
  patch focused on the overlap, not either whole family.
- Start by inspecting the prepared memory/access records and call plans for
  `local_dynamic_member_array.c`.
- If the case requires a general non-local pointer provenance design, stop and
  split before implementing broad support.
