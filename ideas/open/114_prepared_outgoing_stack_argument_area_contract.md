# 114 Prepared Outgoing Stack Argument Area Contract

## Goal

Make the total outgoing stack argument reservation/address area an explicit
prepared call-plan contract that target backends can consume, review, and dump
without deriving the shared area only from per-argument stack destinations.

## Why This Exists

Idea 113 found exactly one unresolved aggregate call-boundary gap after
reviewing idea 112 and nearby closed work: AArch64 has proven that outgoing
stack arguments need stack-area reservation before stack stores, but the shared
prepared contract still exposes per-argument destination offsets/sizes rather
than a first-class total outgoing stack argument area.

That leaves ownership ambiguous. Shared prealloc/call planning should own the
target-neutral fact that a call has an outgoing argument stack area with a
total size, alignment/extent, and addressable destination range. A target
backend should own how that area is physically reserved, which scratch base is
used, which instructions are emitted, and when the stack pointer is restored.

## Owner Boundary

Shared prealloc/call planning owns:

- computing and publishing whether a prepared call needs an outgoing stack
  argument area;
- publishing the total byte extent and alignment/base-address contract needed
  to address all prepared stack argument destinations for that call;
- making the fact visible in prepared dumps and lookups so reviewers can check
  the contract without reconstructing it from individual arguments.

Target backends own:

- physical stack adjustment and restoration;
- scratch-base register selection, including AArch64's `x16` convention;
- concrete load/store instruction ordering and encoding;
- target ABI details for how each stack argument is placed inside the shared
  area.

## Likely Files Or Modules

- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_printer/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp` only as a consumer/proof target,
  not as the source of the shared contract
- `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## In Scope

- Add or refine a prepared call-plan field/helper that explicitly records the
  outgoing stack argument area for a call.
- Ensure the prepared printer exposes the area as a call-level fact separate
  from individual `dest_stack_offset` argument fields.
- Ensure prepared lookup/classification helpers preserve and validate the
  call-level area when classifying call-boundary effects.
- Update AArch64 consumption only enough to use the shared prepared area as the
  reservation/addressing authority while preserving its target-specific
  scratch-base and instruction sequencing decisions.
- Add focused tests proving the shared fact, printer visibility, and one
  target consumer route for stack arguments.

## Out Of Scope

- Moving AArch64's `x16` scratch-base convention into shared prealloc data.
- Prescribing concrete target instruction ordering from the shared contract.
- Reworking source selection, byval aggregate transport, or aggregate `va_arg`
  homes; idea 113 found those already covered by existing closed work.
- Implementing x86 or RISC-V aggregate stack argument lowering.
- Broad call ABI rewrites unrelated to outgoing stack area publication.

## Proof Route

- A BIR/prealloc unit test proves a prepared call with multiple stack argument
  destinations publishes one total outgoing stack area covering all
  destinations.
- A prepared-printer test proves the call-level area appears in dumps
  independently from per-argument stack offsets.
- A classification or lookup test proves the area survives prepared
  call-boundary classification instead of being recomputed ad hoc by a target.
- A focused AArch64 dispatch test proves outgoing stack reservation is driven
  by the prepared area while the emitted `x16` base and store ordering remain
  target-owned.
- The existing c-testsuite AArch64 stack-argument cases remain supported
  without weakening expectations.

## Acceptance And Completion Criteria

- The prepared call-plan surface has an explicit, reviewable outgoing stack
  argument area fact.
- The area fact clearly separates shared total reservation/address-area
  ownership from target scratch-base and instruction-emission policy.
- Tests cover at least one multi-stack-argument shape where deriving the total
  from only the first destination would be wrong.
- Prepared dumps make the shared area visible enough for reviewers to diagnose
  missing reservation/address authority.
- No source-selection, byval, or variadic aggregate-home duplicate initiative
  is added as part of this work.

## Reviewer Reject Signals

- The route adds another AArch64-only reservation variable while claiming to
  solve the shared prepared contract.
- The diff hard-codes `x16`, AArch64 store order, or `sub sp`/`add sp`
  sequencing into shared prealloc structs or prepared dumps.
- The proof only makes `00204`, `00216`, or one named fixture pass without
  demonstrating a call-level area fact that covers nearby stack-argument
  shapes.
- Tests are weakened, downgraded to unsupported, or rewritten to no longer
  require outgoing stack reservation before stack argument stores.
- The implementation keeps the old failure mode hidden behind a helper rename:
  targets still infer the total area from per-argument offsets with no
  explicit prepared fact or dump visibility.
- The route reopens already-covered work for aggregate source selection, byval
  transport, or aggregate `va_arg` homes instead of staying focused on the
  outgoing stack argument area.
- A broad BIR/prealloc call ABI rewrite lands without proving the bounded
  prepared area contract first.
