# 271 Phase F5 x86/riscv memory_accesses public-consumer fixture support

## Closure Note

Closed after the RISC-V dynamic stack-source `LoadLocal` fixture route was
made to reach a real backend consumer that directly reads
`PreparedFunctionLookups::memory_accesses` through normal prepared lookup
construction. The route records the concrete prepared memory row, Route 3/Route
5 authority facts, preserved `lw a1, 12(s2)` compatibility output, and
follow-up fail-closed proof entry point in `todo.md` before close.

Validation accepted for close:
`backend_riscv_prepared_edge_publication` passed (`1/1`) on the accepted
narrow fixture-support scope, and supervisor-recorded extra RISC-V validation
passed (`3/3`). The close-time regression guard used the existing accepted
fixture proof log as both sides of a lifecycle-only non-decreasing comparison
because no code changed during close and the delegated boundary forbade
touching root-level logs or build outputs.

Residual risk intentionally carried forward: no direct x86 public-field
consumer exists yet, so x86 remains adjacent Route 3/Route 5 compatibility
context. This is compatible with the source criterion of an x86 or RISC-V
fixture path.

## Goal

Add or expose supported fixture coverage that lets an x86 or riscv backend
same-consumer proof reach a real consumer of the public
`PreparedFunctionLookups::memory_accesses` field without hand-built stale
prepared state.

## Why This Exists

Closed idea
`ideas/closed/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md`
found no current x86 or riscv proof surface that directly reads
`PreparedFunctionLookups::memory_accesses`. The nearest x86 Route 3/Route 5
path is useful but adjacent: it consumes prepared edge-publication
source-memory and addressing records, not the public memory lookup field. The
nearest exact public-field consumer is AArch64 scalar ALU lowering, outside the
x86/riscv target.

This idea exists to create the missing supported fixture route before any
prepared-only fail-closed proof is attempted.

## In Scope

- Identify the narrowest x86 or riscv backend path that should legitimately
  consume `PreparedFunctionLookups::memory_accesses`.
- Add or expose fixture support so that path reaches a real public-field
  consumer through normal prepared lookup construction.
- Preserve the adjacent x86 Route 3/Route 5 `LoadLocal` compatibility output
  and use it only as adjacent evidence unless the path is changed to consume
  the public memory lookup field.
- Record the exact consumer, prepared memory row, missing or non-agreeing
  Route 3/Route 5 authority facts, and supported fixture path needed for a
  later same-consumer fail-closed proof.
- Keep helper/oracle checks as support only; they are not enough unless a
  target backend consumer also reads the public field.

## Out Of Scope

- Proving the final prepared-only fail-closed behavior before supported
  fixture reachability exists.
- Hand-building stale prepared state that bypasses normal fixture construction.
- Widening the target to AArch64 unless a separate lifecycle decision changes
  the backend target.
- Demoting, deleting, privatizing, wrapping, hiding, or renaming
  `memory_accesses`.
- Weakening helper/oracle statuses, route-debug output, prepared-printer
  output, wrapper output, exact target output, fallback behavior, unsupported
  behavior, or baselines.

## Acceptance Criteria

- A supported x86 or riscv fixture path reaches a backend consumer that
  directly reads `PreparedFunctionLookups::memory_accesses`.
- The fixture can name a concrete prepared memory row and the matching Route 3
  or Route 5 semantic authority facts needed for a later prepared-only
  fail-closed proof.
- Adjacent compatibility output remains byte-stable unless explicitly changed
  by a reviewed contract update.
- The resulting notes are sufficient for a follow-up proof packet to start by
  selecting a same-consumer prepared-only row without synthetic state.
- Matching narrow build/test proof exists for the fixture-support slice.

## Suggested Starting Evidence

- Adjacent x86 fixture:
  `make_x86_param_eq_zero_branch_joined_loadlocal_or_sub_then_add_module()` /
  function `branch_join_loadlocal_then_add`; true predecessor block `is_zero`
  instruction 0 is `LoadLocal` result `zero.loaded` from local slot
  `%contract.selected.source.slot`, BIR byte offset 8, size 4, align 4.
- Compatibility output to preserve for that adjacent x86 row:

```asm
.intel_syntax noprefix
.text
.globl branch_join_loadlocal_then_add
.type branch_join_loadlocal_then_add, @function
branch_join_loadlocal_then_add:
    test edi, edi
    jne .Lbranch_join_loadlocal_then_add_is_nonzero
    sub rsp, 4
    mov eax, DWORD PTR [rsp + 16]
    add eax, 2
    add rsp, 4
    ret
.Lbranch_join_loadlocal_then_add_is_nonzero:
    mov eax, edi
    sub eax, 1
    add eax, 2
    ret
```

- Nearest exact public-field consumer outside scope:
  `tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`
  `scalar_consumers_use_load_local_source_for_unpublished_stack_or_gp_register_results()`
  via AArch64 scalar ALU lowering.

## Reviewer Reject Signals

- The slice claims fixture support while the target x86/riscv backend consumer
  still does not directly read `PreparedFunctionLookups::memory_accesses`.
- The route relies on hand-built stale prepared state or test-only mutation
  that bypasses normal prepared lookup construction.
- The proof is helper-only or oracle-only but is claimed as backend
  same-consumer coverage.
- The patch adds testcase-shaped matching, named-fixture shortcuts, or
  branch-specific special cases instead of a reusable fixture or semantic
  reachability path.
- The patch weakens expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baselines.
- The patch hides, demotes, deletes, privatizes, wraps, or renames
  `memory_accesses` while claiming to add public-consumer fixture support.
- The old failure mode remains, with x86/riscv still consuming only adjacent
  edge-publication or addressing records while the diff renames that path as
  `memory_accesses` coverage.
