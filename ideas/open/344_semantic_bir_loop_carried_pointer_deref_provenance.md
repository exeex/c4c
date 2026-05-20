# Semantic BIR Loop-Carried Pointer Dereference Provenance

Status: Open
Created: 2026-05-20
Split From: ideas/closed/343_aarch64_duff_fallthrough_copy_fixed_offset_skip.md

## Goal

Repair semantic BIR lowering for local pointer dereferences such as
`*from++` and `*to++` so dereferences consume loop-carried pointer-local state
instead of being frozen to direct array-base slots across loopback iterations.

## Why This Exists

Idea 343 repaired the AArch64 Duff fallthrough fixed-offset skip. Prepared BIR
now carries consecutive short-copy offsets, generated AArch64 emits those
consecutive offsets, and the Duff latch still branches on the single
post-decrement counter value repaired by idea 342.

The representative `c_testsuite_aarch64_backend_src_00143_c` still fails
`[RUNTIME_NONZERO]`. Step 3 classification found the new first bad fact before
prepare or AArch64 emission: semantic BIR updates `%lv.from` and `%lv.to`, but
the actual dereferences remain direct base-slot accesses through `%lv.a.0` and
`%lv.b.0`. On loopback, later iterations repeat the first eight array elements
instead of using the advanced pointer-local values.

## In Scope

- Localize where semantic BIR lowering decides that a dereference of a local
  pointer initialized from an array base can remain a direct local-slot access.
- Repair pointer provenance, local GEP, or memory lowering so loop-carried
  pointer locals are consumed when lowering loads and stores through `*p`.
- Preserve direct fixed-offset local-slot lowering when the pointer value is
  not loop-carried or otherwise mutated in a way that changes the dereference
  target.
- Add or update focused coverage for pointer locals initialized from array
  bases, incremented across a loopback, and dereferenced through load and store
  paths.
- Re-run `c_testsuite_aarch64_backend_src_00143_c` after semantic BIR evidence
  shows dereferences consume `%lv.from` and `%lv.to`.

## Out Of Scope

- Fixed-offset fallthrough copy emission repaired by idea 343.
- Duff do-while latch condition decrement/compare emission repaired by idea
  342.
- Scalar-cast register-source publication repaired by idea 340.
- Expectation, unsupported, runner, timeout, proof-log-policy, or CTest
  registration changes.
- Fixing only `00143`, one block number, one stack offset, one local name, one
  source line, or one emitted instruction spelling.
- Broad frame-layout, aggregate, variadic, parser, semantic HIR, or frontend
  rewrites unless fresh evidence proves they own this pointer-provenance
  failure.

## Acceptance Criteria

- The first bad boundary is localized to a concrete semantic BIR memory,
  pointer-provenance, local GEP, or dereference lowering owner.
- Focused coverage proves loop-carried pointer locals initialized from array
  bases are dereferenced through their current pointer state after loopback.
- Direct fixed-offset local-slot lowering still works for non-mutated or
  safely proven fixed local-slot pointers.
- Semantic BIR for the `00143` Duff copy loop no longer lowers later
  `*from++` and `*to++` dereferences as repeated direct `%lv.a.0` and
  `%lv.b.0` base-slot accesses across loopback.
- `c_testsuite_aarch64_backend_src_00143_c` either passes or is reclassified by
  a new first bad fact after loop-carried pointer dereference lowering is
  repaired.
- No expectation, runner, timeout, unsupported classification,
  CTest-registration, or proof-log-policy change is used to claim progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00143`, Duff-device blocks, `%lv.from`, `%lv.to`, `%lv.a.0`,
  `%lv.b.0`, one block number, one stack offset, one source line, or one exact
  emitted instruction sequence instead of repairing pointer dereference
  lowering generally;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to reduce the failure count;
- claims progress only through helper renames, diagnostic rewrites,
  classification notes, or c-testsuite expectation changes;
- reopens fixed-offset copy emission or Duff latch emission without fresh
  evidence that those old first bad facts returned;
- broadens into unrelated frame-layout, aggregate, variadic, parser, semantic
  HIR, or frontend rewrites before proving the semantic pointer-provenance
  boundary;
- leaves semantic BIR still dereferencing loop-carried pointer locals as
  repeated direct base-slot accesses behind a renamed abstraction.
