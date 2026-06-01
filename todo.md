# Current Packet

Status: Active
Source Idea Path: ideas/open/81_aarch64_dispatch_edge_copy_owner_contraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Contract the remaining edge-copy owner surface

## Just Finished

Step 3 is exhausted after the committed target-local helper moves:

- `prepared_memory_access` and `prepared_memory_access_matches_instruction`
  now live in the memory owner.
- `emit_prepared_va_list_field_carrier_to_register` now lives in the
  variadic owner.
- The same-width I32 typed stack-source publication carrier and
  `emit_same_width_i32_stack_source_publication` now live in the memory owner.

Remaining helper families in `dispatch_edge_copies.*` are not valid Step 3
relocation packets without a broader route decision:

- Prepared edge-publication producer lookup and source validation still serve
  edge-copy selection/materialization and should not be moved to a target-local
  emission owner piecemeal.
- `emit_edge_load_local_to_register*` remains coupled to recursive
  edge-value publication, prepared edge-publication memory-source validation,
  and source fallback handling; moving it as a narrow memory packet would
  split prepared edge-source authority rather than clarify ownership.
- `emit_edge_value_publication_to_register*` and `EdgeSelectChainState` remain
  recursive edge-publication/select materialization orchestration.
- `should_emit_block_entry_edge_copy_move`,
  `lower_predecessor_join_source_publication`, and
  `lower_predecessor_select_parallel_copy_sources` are Step 4 surface
  decisions: fold into `dispatch.cpp` only if that makes the public edge-copy
  surface clearer, otherwise document why they remain.

## Suggested Next

Next packet should execute `plan.md` Step 4 with a narrow surface-contraction
decision:

- Inspect the current `dispatch_edge_copies.hpp` public declarations and their
  call sites.
- Delete stale declarations/includes left by the completed Step 3 moves if any
  remain.
- Decide whether the block-entry/select public hooks should fold into
  `dispatch.cpp` as dispatch-owned orchestration, or remain in
  `dispatch_edge_copies.*` with an explicit retained-ownership note.
- Keep prepared source facts consumed directly; do not split
  `emit_edge_load_local_to_register*` or
  `emit_edge_value_publication_to_register*` into another owner unless a
  separate plan review creates a narrower substep.

## Watchouts

- Step 2 was effectively bypassed during the completed Step 3 helper moves.
  Treat any dispatch-only orchestration fold as part of the Step 4 public
  surface decision, not as another open-ended Step 3 relocation.
- Do not rederive prepared edge-copy, prepared edge-publication, or typed
  stack-source facts locally.
- Do not weaken edge-copy, block-entry publication, or typed stack-source test
  expectations.
- Do not pull dispatch-publication, printer, instruction-record, x86, or
  RISC-V cleanup into this route.

## Proof

Latest Step 3 implementation proof passed:

`bash -o pipefail -c "(cmake --build build --target c4c_backend backend_aarch64_instruction_dispatch_test c4cll -- -j2 && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch' --output-on-failure) 2>&1 | tee test_after.log"`

Proof log: `test_after.log`.

No new code proof was required for this plan-owner Step 3 exhaustion decision;
only `todo.md` lifecycle metadata and handoff notes were updated.
