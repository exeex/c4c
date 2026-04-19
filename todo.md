# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Finish Contract-Strict CFG Handoff Surfaces
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2.3 slice for idea 62. The local countdown
renderer in `src/backend/mir/x86/codegen/prepared_countdown_render.cpp` now
prefers authoritative prepared single-successor targets for init carriers,
countdown body backedges, and continuation-entry branch carriers before it
will trust a raw `target_label`, so the two-segment countdown fallback no
longer depends on driftable local branch labels once shared prepare already
owns those CFG edges. The x86 handoff boundary countdown suite now proves the
existing continuation carrier still emits the canonical asm even after its raw
branch target is drifted away from the authoritative prepared successor.

## Suggested Next

Continue `plan.md` Step 2.3 by pushing the remaining countdown control-flow
consumers onto the same strict prepared CFG route, especially the
`render_prepared_loop_join_countdown_if_supported` branch-carrier checks that
still read raw single-successor labels in the loop-join matcher after shared
prepare has already published authoritative branch targets.

## Watchouts

- Do not reintroduce raw string-keyed control-flow contracts now that idea 64
  closed.
- Keep phi-completion work in idea 63 unless it is strictly required to make
  CFG ownership truthful.
- Reject testcase-shaped branch or join matcher growth.
- Keep `PreparedBranchCondition` and `PreparedControlFlowBlock` targets
  contract-consistent; mismatches should still fail the canonical
  prepared-module handoff instead of silently preferring whichever record
  happens to look usable locally.
- The dedicated i32/i16 local guard renderers now also treat a missing
  prepared branch record as a contract failure whenever the entry block already
  has an authoritative prepared control-flow block; keep the remaining
  compare-driven helpers equally strict about not reopening raw guard carriers.
- The generic local-slot short-circuit renderer now prefers authoritative
  prepared passthrough branch targets over drifted raw local labels, but keep
  the remaining branch-carrier consumers equally strict about not reopening
  raw successor recovery once shared prepare publishes a control-flow block.
- `prepared_countdown_render.cpp` now consumes prepared control-flow block
  successor labels even when a local countdown segment does not have a
  dedicated prepared branch-condition record; keep that route strict about
  rejecting mismatched authoritative metadata instead of silently falling back
  to raw terminator drift.
- The remaining loop-join countdown matcher still has a few raw
  single-successor branch-label reads in its bounded carrier checks; keep the
  next packet focused on replacing those with the same prepared-control-flow
  contract instead of widening into unrelated countdown rewrites.

## Proof

Ran the delegated proof command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. Before the full subset
run, `cmake --build --preset default --target backend_x86_handoff_boundary_test`
and `ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$'`
passed with the new continuation-carrier branch-target drift coverage. The
backend subset stayed at the expected baseline with the same four pre-existing
route failures already present in `test_before.log`:
`backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
`backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
and
`backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.
