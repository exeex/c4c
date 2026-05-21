# AArch64 00217 C Behavior Runtime Mismatch

Status: Closed
Created: 2026-05-21
Split From: ideas/closed/367_semantic_bir_indirect_local_memory_lvalue_admission.md
Closed: 2026-05-21

## Goal

Localize and repair the AArch64 runtime mismatch now visible in
`c_testsuite_aarch64_backend_src_00217_c` after semantic BIR admission succeeds.

## Why This Exists

Idea 367 repaired the semantic `lir_to_bir` admission gap for indirect
local-memory lvalues whose effective address comes from loaded pointer values,
pointer-to-pointer locals, pointer arithmetic, or casts. After commit
`b2253656b Admit indirect local memory lvalues`, `00217` no longer fails before
prepared-module or AArch64 handoff. The new first bad fact is later runtime
behavior:

- expected: `data = "0123-5678"`
- actual: `data = "0123\x05"`

The representative source updates a global byte string through casted
pointer arithmetic, `*(unsigned*)(data + r) += a - b`. The residual should be
owned by the first incorrect prepared-BIR, generated-AArch64, ABI, memory,
or runtime boundary that explains why the update corrupts or truncates the
string output after semantic admission has succeeded.

## In Scope

- Localize the first incorrect boundary for `00217` after semantic BIR handoff,
  comparing semantic BIR, prepared BIR, generated AArch64, and runtime output.
- Determine whether the mismatch comes from pointer-derived address formation,
  cast-width load/store behavior, scalar update value materialization,
  memory writeback, string/global storage publication, or another concrete
  AArch64/runtime owner.
- Add focused backend or generated-code coverage for the localized owner when
  a small contract can represent the failing shape independently of `00217`.
- Repair the general backend/runtime capability for the localized owner without
  matching the filename, the string literal, or the exact output text.
- Prove `00217` advances beyond the current `data = "0123\x05"` mismatch or
  passes, while keeping the idea 367 semantic admission coverage, `00005`, and
  `00173` stable.

## Out Of Scope

- Reopening semantic `lir_to_bir` indirect local-memory admission from idea 367
  unless fresh evidence shows the exact old semantic handoff failure returned.
- Reopening the completed `00173` pointer-derived string-load/publication chain
  unless fresh evidence shows its old failure returned.
- Editing expectations, unsupported classifications, runner behavior, timeout
  policy, CTest registration, proof-log policy, or external test contracts.
- Broad ABI composite/byval/HFA/f128 work, variadic/floating work, dynamic
  stack work, aggregate writeback buckets, timeout/output-storm residuals, or
  unrelated AArch64 runtime mismatches without fresh first-bad-fact evidence
  and a lifecycle split.

## Acceptance Criteria

- The first bad fact is localized to a concrete post-semantic boundary with
  evidence tying that boundary to the observed `00217` output mismatch.
- The repair is a general backend/runtime capability fix for that boundary, not
  a named-case patch for `00217`, `data`, one string literal, one cast spelling,
  one offset, or one emitted instruction neighborhood.
- Focused backend or generated-code coverage exists for the repaired shape when
  practical; if direct focused coverage is not practical, `todo.md` records why
  and names the stronger runtime or generated-code proof used instead.
- `c_testsuite_aarch64_backend_src_00217_c` advances beyond the current
  `data = "0123\x05"` mismatch or passes without expectation, runner, timeout, or
  filename-specific changes.
- Idea 367 coverage remains stable: `backend_lir_to_bir_notes`,
  `c_testsuite_aarch64_backend_src_00005_c`, and
  `c_testsuite_aarch64_backend_src_00173_c` remain passing.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00217`, `data`, the exact expected or actual output, one cast
  spelling, one string literal byte, one source offset, one generated temporary,
  or one emitted instruction neighborhood instead of repairing a general
  localized backend/runtime rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, classification notes, emitted-text
  rewrites, or pass-count movement while the same `data = "0123\x05"` mismatch
  remains unexplained at the first bad boundary;
- reopens semantic BIR admission, the completed `00173` path, ABI composite,
  variadic/floating, dynamic stack, aggregate writeback, timeout/output-storm,
  or unrelated runtime mismatch work without fresh evidence and a lifecycle
  split;
- hides the same runtime corruption behind a broader rewrite that cannot name
  the first incorrect prepared-BIR, AArch64, memory, ABI, or runtime boundary.

## Closure Notes

Closed after commit `9977c2d2c` repaired the localized AArch64
pointer-value local store writeback owner by using the prepared/emitted named
stored-value register instead of recomputing the producer chain during the
writeback fallback. The focused acceptance proof showed
`c_testsuite_aarch64_backend_src_00217_c` passing with
`c_testsuite_aarch64_backend_src_00005_c`,
`c_testsuite_aarch64_backend_src_00173_c`, `backend_lir_to_bir_notes`,
`backend_aarch64_instruction_dispatch`, and the focused
`backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value`
guard stable.

Close guard command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00005_c|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00217_c)$'
```

The guard compared against the existing matching three-test baseline and
passed: `00217` resolved from failing to passing, with no new failing tests.
