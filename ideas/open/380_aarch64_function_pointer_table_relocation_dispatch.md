# AArch64 Function Pointer Table Relocation Dispatch

Status: Open
Created: 2026-05-21
Split From: ideas/closed/379_aarch64_local_aggregate_copy_load_publication.md

## Goal

Repair AArch64 lowering for function-pointer table initializers with multiple
relocation entries so indexed calls dispatch to the distinct functions named by
the source table.

## Why This Exists

Idea 379 advanced `00216.c` past the local aggregate copy/load publication
family. The remaining first bad fact is now in `test_multi_relocs`: the
program prints `two/two/two` instead of `one/two/three` for this table:

```c
const fptr table[3] = {
    [0 ... 2] = &sys_ni,
    [0] = sys_one,
    [1] = sys_two,
    [2] = sys_three,
};
```

`ls21`, `lu22`, `lv3`, and `flow` now match expected output, so this should
not be folded back into local aggregate copy publication without fresh evidence
of a shared lowering boundary.

## In Scope

- Localize whether the mismatch comes from semantic initializer handling,
  designated/range initializer overwrite order, global relocation emission,
  function-pointer table object layout, AArch64 address materialization, or the
  indexed indirect-call path.
- Trace `table[0]`, `table[1]`, and `table[2]` from semantic BIR through
  prepared BIR, MIR/global data, relocations, generated AArch64 assembly, and
  runtime dispatch.
- Repair the general table/relocation lowering rule so multiple function
  pointer entries preserve their distinct relocation targets.
- Add or use focused backend coverage independent of the external `00216.c`
  filename.
- Prove `test_multi_relocs` prints `one`, `two`, `three` in order without
  regressing the aggregate-publication guardrails fixed under idea 379.

## Out Of Scope

- Local aggregate address/copy/load publication, pointer-published aggregate
  homes, and fixed aggregate slice offsets already owned by idea 379.
- Timeout-only `00200` and `00207` routes.
- Rewriting external test expectations, unsupported classifications,
  allowlists, runner behavior, timeout policy, CTest registration, proof-log
  policy, or the c-testsuite source.
- Special-casing `00216`, `test_multi_relocs`, `sys_one`, `sys_two`,
  `sys_three`, the `table` symbol, one relocation index, one emitted label, or
  one generated instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete initializer, relocation,
  global-data, address-materialization, or indirect-call dispatch boundary.
- Focused backend coverage demonstrates a multi-entry function-pointer table
  relocation case without depending on the `00216.c` filename.
- The focused case preserves distinct targets for at least three function
  pointer table entries, including overwrite behavior after a range
  initializer when relevant.
- `c_testsuite_aarch64_backend_src_00216_c` advances past the current
  `test_multi_relocs` `two/two/two` mismatch or records a new first bad fact
  in `todo.md`.
- Backend-focused proof keeps the 146 backend tests passing and preserves the
  aggregate outputs repaired under idea 379.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216`, `test_multi_relocs`, `sys_one`, `sys_two`,
  `sys_three`, `table`, one table length, one relocation index, one label name,
  or the exact emitted call sequence instead of repairing function-pointer
  table relocation dispatch generally;
- weakens expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or external
  test contracts;
- claims progress through helper renames, emitted-text reshuffling,
  classification-only notes, or expectation movement while a multi-entry
  function-pointer table can still collapse distinct targets to one function;
- broadens back into aggregate copy/load publication, timeout-only routes, or
  unrelated global initializer work without fresh first-bad-fact evidence and a
  lifecycle handoff;
- proves only the external representative while leaving focused backend
  coverage for function-pointer table relocation dispatch absent or failing;
- retains the exact old `two/two/two` failure mode behind a renamed lowering
  helper or new abstraction boundary.
