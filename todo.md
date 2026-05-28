Status: Active
Source Idea Path: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish The Blocked Failure-Family Baseline

# Current Packet

## Just Finished

Lifecycle switched from idea 58's stuck Step 3 recovery route to the
decomposition idea in
`ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`.

Preserved incomplete dirty implementation context:

- `src/backend/mir/aarch64/codegen/memory.cpp` and `memory.hpp`: store-global
  pending publication ownership/accounting and selected local-store accounting
  context.
- `src/backend/mir/aarch64/codegen/dispatch.cpp`: dispatch accounting context
  for prepared publications.
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` and
  `dispatch_publication.hpp`: selected fused-compare scratch preference.
- `src/backend/mir/aarch64/codegen/calls.cpp`: call/outgoing stack argument
  materialization fallbacks.
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`: direct edge
  `LoadLocal` prepared source-memory consumption.

These edits are preserved as incomplete implementation context. They are not
accepted recovery progress until the decomposed focused probes and supervisor
validation make the relevant slices acceptance-ready.

## Suggested Next

Execute Step 1 by recording the blocked baseline from `test_after.log` and
`review/step3_dispatch_route_review.md`, then proceed to Step 2 seam inventory.
Do not delegate more backend implementation until a focused probe and backend
owner are selected.

## Watchouts

- Idea 58 remains open and incomplete:
  `backend_aarch64_instruction_dispatch` and
  `c_testsuite_aarch64_backend_src_00196_c` still fail.
- The focused proof was last reported as `2/4` passing:
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor` and
  `c_testsuite_aarch64_backend_src_00207_c` passed.
- The latest dispatch blocker is
  `expected GOT-required global load to use GOT page/low12 materialization`.
- The route drift evidence is that the first bad dispatch assertion moved
  across store-global publication ownership, fused compare materialization,
  call/outgoing stack arguments, direct edge `LoadLocal` publication, and now
  GOT-required `LoadGlobal` materialization.
- `review/step3_dispatch_route_review.md` is transient review context; keep it
  read-only unless the supervisor delegates another review.

## Proof

Lifecycle-only switch. No build or ctest proof was run for this plan-owner
packet.
