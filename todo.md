Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Expose Authorized Before-Instruction Move Bundle To Object Route

# Current Packet

## Just Finished

Completed Step 6 for idea 456. Summary artifact:
`build/agent_state/456_step6_before_instruction_move_bundle/summary.md`.

Implemented the narrow prepared object traversal and RV64 consumer route for
the authorized before-instruction cast-dependency stack publication:

- added `PreparedObjectTraversalEventKind::BeforeInstructionCopies`;
- traversal emits only before-instruction move bundles with stack destinations;
- RV64 consumes that event through the existing move-bundle path;
- RV64 suppresses only the exact `consumer_register_to_stack` publication whose
  move is `cast_source_value_id -> dependency_value_id` for a populated
  `rematerialize_cast_from_source status=available` authority record;
- authority matching uses prepared control-flow block labels, not raw BIR block
  label ids.

Focused coverage now proves the accepted stack-publication suppression path and
rejected boundaries for missing cast producer/authority, unsupported stack-slot
cast source homes, extra non-carrier cast uses, mismatched stack-publication
source identity, mismatched dependency operand identity, and the existing
LHS-authority/RHS-`t3` scratch-clobber guard.

Fresh `20010329-1` probes under
`build/agent_state/456_step6_before_instruction_move_bundle/` still show
`prepared_exit=0` and `object_exit=2` with the broad
`unsupported_move_bundle_target_shape` diagnostic. The before-instruction stack
publication is no longer the local blocker; Step 7 should classify the later
move-bundle residual.

## Suggested Next

Step 7: `Final Residual Disposition And Close Readiness`.

Re-probe and classify the remaining `20010329-1` object-route
`unsupported_move_bundle_target_shape` after the explicit cast-dependency
compare consumer and the authorized before-instruction stack publication
suppression are both in place. Decide whether any exact idea-456 packet remains
or whether the next owner is a separate later move-bundle/materialization
family.

Suggested owned files:

- `todo.md`
- `test_after.log`
- `build/agent_state/456_step7_final_residual_disposition/*`

Suggested proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Full `20010329-1` object success remains unclaimed.
- The accepted Step 6 traversal event is intentionally limited to
  before-instruction stack-destination bundles; RV64 suppresses only the
  authorized cast-dependency stack publication.
- The remaining broad move-bundle diagnostic should be reclassified before any
  further implementation packet is selected.
- Continue to consume only populated `rematerialize_cast_from_source
  status=available` dependency-operand authority.
- Preserve the scratch-clobber guard unless a later packet implements a
  register-safe materialization ordering or scratch allocator.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
- Do not copy successor/join-block compare results on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, baseline logs, or `review/`.

## Proof

Step 6 proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

Result: passed; proof output is in `test_after.log`.
