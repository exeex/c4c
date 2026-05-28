Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish The Blocked Failure-Family Baseline

# Current Packet

## Just Finished

Completed Step 1 - Establish The Blocked Failure-Family Baseline for the active
decomposition route in
`ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`.

Baseline recorded from existing evidence only:

- Active route: decomposition is now the active plan; idea 58 remains open and
  incomplete, with its four-test recovery criteria not accepted or closed.
- Focused proof result in `test_after.log`: `2/4` passing.
- Passing tests:
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` and
  `c_testsuite_aarch64_backend_src_00207_c`.
- Failing tests: `backend_aarch64_instruction_dispatch` and
  `c_testsuite_aarch64_backend_src_00196_c`.
- Latest first bad dispatch assertion:
  `expected GOT-required global load to use GOT page/low12 materialization`.
- `00196` still reports the known runtime mismatch and remains separate unless
  decomposition proves shared ownership with a dispatch/prepared-publication
  seam.

Current dirty implementation context from `git status --short` is incomplete
route evidence, not accepted recovery progress:

- `src/backend/mir/aarch64/codegen/memory.cpp` and `memory.hpp`: store-global
  pending publication ownership/accounting plus selected local-store accounting
  context.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`: dispatch prepared-publication
  accounting context.
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` and
  `dispatch_publication.hpp`: selected fused-compare scratch preference.
- `src/backend/mir/aarch64/codegen/calls.cpp`: call/outgoing stack argument
  materialization fallbacks.
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`: direct edge
  `LoadLocal` prepared source-memory consumption.
- `review/step3_dispatch_route_review.md`: transient review artifact that
  judged the dirty stack non-overfit but route-drifting and recommended
  decomposition before further unrelated implementation.

The blocked route has moved across store-global publication ownership,
store-local/dispatch accounting, fused-compare materialization, call/outgoing
stack argument materialization, direct edge `LoadLocal` source-memory
consumption, and now GOT-required `LoadGlobal` materialization. This is the
blocked failure family that Step 2 must inventory before implementation
resumes.

## Suggested Next

Execute Step 2 - Inventory The Separable Dispatch Seams.

Suggested Step 2 seam inventory should map each visible seam to an owner surface
and evidence source:

- Store-global publication ownership: `memory.cpp`/`memory.hpp` plus dispatch
  publication accounting evidence from the dirty stack and review.
- Store-local publication ownership: `memory.cpp` future-consumer/accounting
  context, with the review's drift warning as evidence.
- Fused-compare materialization: `dispatch_publication.cpp`/`.hpp`, with the
  selected global-load compare-order assertion as evidence.
- Call/outgoing stack arguments: `calls.cpp`, with prepared immediate stack
  call-argument and aggregate frame-slot assertions as evidence.
- Direct edge `LoadLocal` publication: `dispatch_edge_copies.cpp`, with the
  prepared source-memory consumption assertion as evidence.
- GOT-required `LoadGlobal` materialization: global-load materialization helpers
  around the current first bad assertion in `test_after.log`.

Keep `00196` separate during Step 2 unless concrete evidence ties it to one of
these seams.

## Watchouts

- Do not resume implementation from the next monolithic
  `backend_aarch64_instruction_dispatch` assertion until Step 2 inventories the
  seam and Step 3/4 bind it to a focused probe and owner.
- The dirty stack is intentionally preserved. Do not revert it as part of
  decomposition documentation, but do not classify it as accepted progress yet.
- `review/step3_dispatch_route_review.md` described the dirty route as
  non-overfit but drifting; that is a warning, not acceptance.
- Do not weaken tests, add testcase-name checks, or claim todo-only
  classification as backend capability progress.

## Proof

No fresh test proof required or run for this documentation-only Step 1 packet.

Evidence used:

- Existing `test_after.log`, showing the current four-test focused proof at
  `2/4` passing and the latest dispatch assertion.
- Existing `review/step3_dispatch_route_review.md`, showing route drift without
  blocking testcase-overfit signals.
- Current `git status --short`, showing the dirty implementation surfaces and
  transient review artifact.
- Active `plan.md` and source idea 60, confirming decomposition is the active
  route.

Proof log path: `test_after.log`.
