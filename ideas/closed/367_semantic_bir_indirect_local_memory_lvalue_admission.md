# Semantic BIR Indirect Local-Memory Lvalue Admission

Status: Closed
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Closed: 2026-05-21

## Goal

Repair semantic `lir_to_bir` lowering for indirect local-memory lvalues whose
address comes from loaded pointer values, pointer-to-pointer locals, pointer
arithmetic, or casts.

## Why This Exists

The post-365/366 backend-regex classification found that the strongest current
compile-stage owner is not another AArch64 runtime, printer, runner, timeout,
or expectation bucket. Two external representatives stop before prepared
AArch64 handoff in semantic `lir_to_bir`:

- `c_testsuite_aarch64_backend_src_00005_c` reports a store local-memory
  failure for a pointer-to-pointer local shape, represented by `**pp = 1`.
- `c_testsuite_aarch64_backend_src_00217_c` reports a load local-memory
  failure for pointer arithmetic plus casted unsigned update,
  represented by `*(unsigned*)(data + r) += a - b`.

Both failures point at admission and representation of indirect local-memory
loads and stores whose effective address is computed from a pointer value, not
at emitted AArch64 assembly behavior.

## In Scope

- Localize the semantic `lir_to_bir` decision that rejects or misclassifies
  indirect local-memory lvalues backed by loaded pointer values.
- Repair both load and store forms for pointer-derived local-memory lvalues.
- Cover pointer-to-pointer locals, pointer arithmetic, and cast-derived
  lvalue addresses enough to prove the rule is semantic rather than
  filename-shaped.
- Add focused semantic or backend-route coverage showing indirect load and
  store lvalues remain represented as real pointer-based memory operations.
- Prove `c_testsuite_aarch64_backend_src_00005_c` and
  `c_testsuite_aarch64_backend_src_00217_c` advance past semantic handoff or
  pass.

## Out Of Scope

- AArch64 register allocation, instruction selection, machine printing,
  assembler legality, linker behavior, runtime mismatches, runner policy,
  timeout policy, CTest registration, proof-log policy, unsupported
  classification changes, or external expectation changes.
- Local backend-route snippet rewrites or expectation cleanup unless the
  supervisor delegates a separate test-contract cleanup.
- Reopening the completed `00173` pointer-derived string-load/publication
  chain unless fresh evidence shows the exact old semantic or publication
  failure returned.
- Composite/byval/HFA/f128 ABI work, variadic floating work, aggregate
  writeback runtime buckets, scalar compare/select publication, initializer
  residuals, dynamic stack work, or timeout/output-storm residuals.

## Acceptance Criteria

- The first bad boundary is localized to a concrete semantic `lir_to_bir`
  indirect local-memory load/store admission rule.
- Focused coverage fails before the repair and passes after it for both
  indirect load and indirect store lvalues.
- `c_testsuite_aarch64_backend_src_00005_c` and
  `c_testsuite_aarch64_backend_src_00217_c` advance past semantic handoff or
  pass without filename-specific handling.
- Adjacent passing pointer-derived load coverage, including the completed
  `00173` path, remains passing.
- No expectation, unsupported-classification, runner, timeout, CTest
  registration, proof-log, or AArch64 runtime-policy change is used to claim
  progress.

## Completion Note

Commit `b2253656b Admit indirect local memory lvalues` completed the semantic
admission owner. The repair admits computed pointer-address local-memory
lvalues for both the loaded pointer/pointer-to-pointer store shape and the
casted byte-pointer/pointer-arithmetic load/update shape without expectation,
unsupported-classification, runner, timeout, CTest-registration, proof-log, or
AArch64 runtime-policy changes.

Focused before/after regression guard for the idea scope passed: the four-test
subset improved from 1/4 to 3/4, resolving `backend_lir_to_bir_notes` and
`c_testsuite_aarch64_backend_src_00005_c` with no new failures.
`c_testsuite_aarch64_backend_src_00173_c` remained passing.

`c_testsuite_aarch64_backend_src_00217_c` advanced past the semantic
`lir_to_bir` handoff required by this idea and now fails later as an AArch64
runtime mismatch:

- expected: `data = "0123-5678"`
- actual: `data = "0123\x05"`

That residual is outside this semantic admission owner and is split to
`ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md`.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00005`, `00217`, `pp`, `data`, one cast spelling, one pointer
  depth, one emitted BIR name, or one source expression instead of repairing a
  general semantic local-memory lvalue admission rule;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, CTest registration, or external test contracts;
- claims progress through helper renames, dump spelling changes,
  classification notes, or local backend-route expectation rewrites while
  semantic `lir_to_bir` still rejects the same indirect local-memory load or
  store family;
- broadens into AArch64 ABI, register allocation, machine printing, aggregate
  writeback, variadic/floating, runtime mismatch, initializer, dynamic stack,
  or timeout work without fresh first-bad-fact evidence and a lifecycle split;
- leaves indirect local-memory lvalues unable to use a loaded pointer value,
  pointer-to-pointer local, pointer-arithmetic result, or cast-derived address
  as the actual memory address behind a new abstraction name.
