# Execution State

Status: Active
Source Idea Path: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Finish Contract-Strict CFG Handoff Surfaces
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 2.3 slice for idea 62. The loop-join
countdown matcher in `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`
now resolves entry carriers, transparent-prefix predecessors, init handoff
branches, and loop body backedges from authoritative prepared single-successor
targets instead of trusting raw `terminator.target_label` drift after prepare.
The x86 handoff boundary countdown suite now proves both the minimal loop join
and the preheader-chain route still emit the canonical asm even when those raw
single-successor labels are mutated away from the prepared CFG contract.

## Suggested Next

Continue `plan.md` Step 2.3 by auditing the remaining local countdown fallback
paths in `src/backend/mir/x86/codegen/prepared_countdown_render.cpp` that still
drop to raw `target_label` recovery when a strict prepared single-successor or
compare-target contract should already be authoritative, especially the
two-segment matcher paths around continuation-entry/body successor discovery.

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
- The remaining raw `target_label` reads are now concentrated in the broader
  local countdown fallback matcher rather than the loop-join handoff path;
  keep the next packet focused on whether those sites belong in Step 2.3
  contract-hardening or in the wider Step 3 consumer migration.

## Proof

Ran the delegated proof command
`cmake --build --preset default --target backend_x86_handoff_boundary_test && ctest --test-dir build -j --output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
and wrote the canonical proof log to `test_after.log`. The focused
`backend_x86_handoff_boundary` proof passed after adding drift coverage for the
loop-join entry/body single-successor carriers and for the transparent
preheader-chain carrier path. `test_after.log` matched `test_before.log`
except for the expected real-time duration line.
