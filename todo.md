# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prepared_call_plan_cursor_complete_production.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce one cursor-exact plan per supported call

## Just Finished

- Completed plan Step 1's missing-call producer trace. The two semantic BIR
  calls in `make_x86_direct_extern_call_lane_module` remain at block 0,
  instruction cursors 0 (`actual_function`, zero arguments, result `%t0`) and
  1 (`printf`, arguments `@.str0`/`%t0`, result `%t1`, variadic). The earliest
  incorrect fact is not cursor renumbering: `populate_call_plans` in
  `src/backend/prealloc/call_plans.cpp` asks `find_call_argument` for every
  argument and drops the entire call when
  `call_argument_boundary_matches_prepared_source` rejects that boundary.
  `find_call_argument` in `src/backend/bir/bir_call_boundary_view.cpp` marks an
  otherwise valid semantic argument `Incomplete` when no matching
  `CallArgumentSourceRelationship` exists. This direct-BIR fixture supplies
  semantic `args` but no optional `arg_sources`, so cursor 1 is omitted; cursor
  0 bypasses the argument loop and is published correctly. Thus the durable
  idea's observation that the first call is omitted and the later call is
  published at cursor 0 is inverted relative to the current producer code.
- The general repair rule is to enumerate every valid semantic `CallInst` at
  its actual block/instruction cursor and derive the base argument identity
  from `CallInst::args[arg_index]`; an absent optional source-relationship row
  must not erase the call. A unique relationship may refine source/base ID,
  selection, aggregate-lane, and producer identity. Duplicate/ambiguous rows,
  out-of-range rows, or a relationship that contradicts the semantic operand
  must continue to fail closed. Preserve callee/link identity, indirect callee,
  wrapper/variadic ABI classification, argument type and ABI destination,
  result/result ABI, before/after-call move bundles, clobbers, and preserved
  values unchanged.
- Nearby coverage under the same rule includes the same-module, fixed-extern,
  variadic-extern, and indirect argument-bearing direct-BIR calls in
  `prepare_call_wrapper_dump_module`; all are constructed without
  `arg_sources` and are expected to publish exact cursor plans. Lowered calls
  with unique explicit `arg_sources` exercise the refinement path. Negative
  states are duplicate relationships for one argument (`Ambiguous`), stale or
  wrong `arg_index`, relationship/operand identity disagreement, invalid call
  boundary/callee identity, and missing/duplicate/stale/callee-mismatched
  prepared plans at lookup.

## Suggested Next

- Execute plan Step 2 as one bounded common-producer packet: adjust
  `bir_call_boundary_view`/`populate_call_plans` argument handling so a valid
  semantic operand is available without an optional relationship, while a
  unique relationship refines it and ambiguous or contradictory evidence
  rejects. Preserve the already-exact enclosing instruction cursor. Add no
  callee, fixture, zero-argument, or two-call special case; prove exact plans
  for both direct-extern cursors plus the nearby fixed, variadic, and indirect
  wrapper shapes.

## Watchouts

- Preserve exact cursor/callee fail-closed validation; the rejected idea-708
  implementation was reverted and is not implementation-complete.
- Do not "fix" cursor accounting: `populate_call_plans` already copies the
  enclosing semantic `instruction_index`. The apparent later-call-at-zero
  symptom comes from the surviving vector position being mistaken for a
  semantic cursor, not from the stored cursor field.
- Preserve ambiguity as a negative state. Treat only absence of optional
  relationship metadata as compatible with the semantic operand; do not
  select the first duplicate relationship or synthesize route authority.

## Proof

- Diagnosis-only packet; no build or test was required and no canonical log was
  written. Evidence came from AST-backed definition/callee queries for
  `populate_call_plans`, targeted inspection of the direct-extern fixture and
  `BirCallBoundaryView`, and a nearby wrapper-fixture cross-check. An existing
  `backend_prepared_printer_test` binary was sampled non-mutatingly but stopped
  at an unrelated earlier intrinsic-carrier failure, so it is not proof for
  this packet.
