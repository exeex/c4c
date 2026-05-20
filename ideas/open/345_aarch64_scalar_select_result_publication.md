# AArch64 Scalar Select Result Publication

Status: Open
Created: 2026-05-20
Split From: ideas/closed/344_semantic_bir_loop_carried_pointer_deref_provenance.md

## Goal

Repair AArch64 scalar select result publication so stack-homed select values
are materialized before later store or consumer reloads use their homes.

## Why This Exists

Idea 344 repaired and reclassified the `00143` residual. Semantic and prepared
BIR now show the Duff copy loop loading `%lv.from` and `%lv.to`, advancing the
loaded pointer values, and dereferencing through pointer-value addresses
instead of stale direct `%lv.a.0` and `%lv.b.0` base slots.

`c_testsuite_aarch64_backend_src_00143_c` still fails at runtime. The new first
bad fact is in AArch64 scalar select result publication/result materialization:
prepared BIR has stack-homed `i16` select results such as `%t9.store0` feeding
`bir.store_local %lv.a.0`, but generated AArch64 computes the select with
`csel` and then immediately reloads the select-result home before storing to
the array slot. The reload observes an unpublished stack home rather than the
fresh selected register value.

## In Scope

- Localize the AArch64 scalar select lowering path that computes a `csel`
  result without publishing it to the stack home expected by later store-local
  or consumer reloads.
- Repair `lower_scalar_select_publication` and its interaction with store-local
  value publication so selected scalar results are available from their
  modeled homes when later instructions reload them.
- Cover short integer select results that feed local array stores or similar
  stack-homed consumers.
- Re-run `c_testsuite_aarch64_backend_src_00143_c` after the select-publication
  evidence is repaired, while preserving the semantic pointer-local dereference
  repair from idea 344.

## Out Of Scope

- Semantic BIR loop-carried pointer dereference provenance repaired by idea
  344.
- Duff fixed-offset fallthrough copy emission repaired by idea 343.
- Duff do-while latch condition decrement/compare emission repaired by idea
  342.
- Scalar-cast register-source publication repaired by idea 340 unless fresh
  evidence proves the same old diagnostic or owner has returned.
- Expectation, unsupported, runner, timeout, proof-log-policy, or CTest
  registration changes.
- Fixing only `00143`, one temporary name, one stack offset, one source line,
  one block number, or one emitted instruction string.

## Acceptance Criteria

- The first bad boundary is localized to a concrete AArch64 select/result
  publication owner.
- Generated AArch64 publishes scalar `csel` results to the modeled home before
  any store-local or consumer path reloads that home.
- Focused coverage proves a stack-homed scalar select result can feed a local
  store or equivalent consumer without reading an unpublished home.
- `c_testsuite_aarch64_backend_src_00143_c` either passes or is reclassified by
  a new first bad fact after scalar select result publication is repaired.
- The semantic pointer-local dereference evidence from idea 344 remains intact:
  Duff copy dereferences continue to use current pointer-local values, not
  stale direct base slots.
- No expectation, runner, timeout, unsupported classification,
  CTest-registration, or proof-log-policy change is used to claim progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, `%t9.store0`, `%t13.store0`, `%lv.a.0`, one stack
  offset, one block number, one source line, or one exact emitted instruction
  sequence instead of repairing scalar select result publication generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- reopens semantic pointer dereference lowering, fixed-offset fallthrough copy
  emission, or Duff latch emission without fresh evidence that those old first
  bad facts returned;
- broadens into unrelated frame layout, aggregate, variadic, parser, semantic
  HIR, or frontend rewrites before proving the scalar select publication
  boundary;
- leaves generated AArch64 still reloading an unpublished stack home for a
  freshly computed scalar select result behind a renamed abstraction.
