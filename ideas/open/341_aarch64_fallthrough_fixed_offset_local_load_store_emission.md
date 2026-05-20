# AArch64 Fallthrough Fixed-Offset Local Load Store Emission

Status: Open
Created: 2026-05-20
Split From: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md

## Goal

Repair AArch64 generated-code emission for BIR blocks that contain
fixed-offset local loads and stores after switch/Duff-device fallthrough so the
emitted assembly performs the data copy, not only pointer-local advancement.

## Why This Exists

Idea 340 repaired the scalar-cast structured register source publication
failure for `00143`: the old machine-printer diagnostic is absent and the
representative now assembles, links, and runs. The remaining failure is runtime
nonzero, and classification evidence points to a distinct generated-code
emission owner.

Prepared BIR for the Duff-device copy contains the expected fallthrough work in
`block_9` through `block_15`: each block loads `a[1]` through `a[7]` and stores
the loaded value to `b[1]` through `b[7]`. Generated AArch64 for the
corresponding fallthrough labels only updates the `from` and `to` pointer
locals and branches onward. The expected `ldrh`/`strh` copy operations are
omitted, so `b[1]` and later elements remain stale when `count % 8 == 7`.

## In Scope

- Localize where fixed-offset local load/store operations in fallthrough BIR
  blocks are dropped before or during AArch64 instruction emission.
- Repair the semantic lowering or emission path so those loads and stores are
  emitted for generated-code fallthrough blocks.
- Add or update focused backend coverage for fallthrough blocks containing
  fixed-offset local loads and stores.
- Prove the focused backend behavior and re-run the representative
  `c_testsuite_aarch64_backend_src_00143_c`.

## Out Of Scope

- Scalar-cast register-source publication, selected scalar cast operand
  normalization, and the closed diagnostic repaired by idea 340.
- Expectation, unsupported, runner, timeout, proof-log-policy, or CTest
  registration changes.
- Fixing only `00143`, one label, one source line, one block number, one
  pointer-local name, or one emitted instruction spelling.
- Broad frame-layout, aggregate, variadic, compare, parser, semantic HIR, or
  frontend rewrites unless fresh evidence proves they own the first bad fact.

## Acceptance Criteria

- The first bad boundary is localized to a concrete BIR-to-AArch64 lowering,
  scheduling, selected-node, or printer-emission owner for fixed-offset local
  loads/stores in fallthrough blocks.
- Focused backend coverage proves fallthrough fixed-offset local loads and
  stores survive to generated AArch64 instruction emission.
- `c_testsuite_aarch64_backend_src_00143_c` either passes or is reclassified
  by a new first bad fact after the missing `ldrh`/`strh` copy operations are
  repaired.
- No expectation, runner, timeout, unsupported classification,
  CTest-registration, or proof-log-policy change is used to claim progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, Duff-device labels, `block_9` through `block_15`,
  selected source lines, `from`/`to` local names, or one fixed instruction
  sequence instead of repairing fallthrough fixed-offset local load/store
  emission generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- reopens scalar-cast register-source publication without fresh evidence that
  the old structured-source diagnostic returned;
- broadens into unrelated frame-layout, aggregate, variadic, compare, parser,
  semantic HIR, or frontend rewrites before proving the fallthrough load/store
  emission boundary;
- leaves prepared BIR load/store operations present but generated AArch64
  fallthrough labels still omitting the corresponding data movement behind a
  renamed abstraction.
