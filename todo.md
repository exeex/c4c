Status: Active
Source Idea Path: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate The Identity Authority Boundary

# Current Packet

## Just Finished

Step 2 of `plan.md` is complete: located the explicit home/value identity
authority boundary for the `tests/c/external/gcc_torture/src/990127-1.c`
same-block RHS `%lv.a` row.

Boundary classification: producer identity missing due to prepared value-id
sentinel mismatch. The row has a real prepared home for `%lv.a`, but that home
uses `value_id=0`: `home %lv.a value_id=0 kind=stack_slot slot_id=18
offset=104`. Branch stack-load publication treats `PreparedValueId{0}` as
invalid before publishing or selecting freshness, so
`branch_stack_load_authority function=main block=block_1 role=rhs value=%lv.a
value_id=0 ... status=home_value_mismatch source_freshness_status=no_candidate
source_freshness_candidates=0` is producer identity fallout, not stale generic
source freshness.

AST-backed symbols inspected:
`prepared_home_names_value`,
`publish_prepared_branch_stack_source_freshness_candidate`,
`plan_prepared_branch_stack_load_authority`,
`make_branch_stack_load_authority_record`,
`selected_branch_stack_load_source_freshness_status`,
`selected_rhs_branch_stack_load_source_freshness_status`,
`fragment_for_prepared_compare_branch`, `BirPreAlloc::run_liveness`,
`BirPreAlloc::run_regalloc`, and `classify_prepared_value_home`.

Why this is not generic source freshness: the producer refuses candidate
publication before the `BranchStackLoadSource` freshness query because
`publish_prepared_branch_stack_source_freshness_candidate(...)` rejects
`inputs.value_home->value_id == PreparedValueId{0}`, and
`plan_prepared_branch_stack_load_authority(...)` rejects the same identity via
`prepared_home_names_value(...)` requiring `home.value_id != 0`. The RV64
consumer has a parallel fail-closed zero-id guard and otherwise matches the
expected role/block/value/home/freshness tuple; it cannot consume a fact the
producer never legally publishes.

## Suggested Next

Step 3 should take one semantic repair route: reconcile `PreparedValueId{0}`
sentinel semantics before branch stack-load identity publication, preferably by
allocating real liveness/prepared value ids from `1` so zero remains the
sentinel already assumed by publication, lookup, object traversal, and RV64
fail-closed checks. Do not patch only the `%lv.a` branch row or infer identity
from stack offset/materialization shape. If global value-id allocation is too
broad for this plan, reclassify the owner to prepared value-id sentinel
allocation instead of RV64 consumer key mismatch.

## Watchouts

- Do not infer value identity from stack offsets, final assembly, source
  syntax, local names, or diagnostics.
- Do not reopen prepared branch stack-source freshness publication from idea
  636.
- Do not absorb RV64 terminator-fragment work from the now-closed idea 645.
- Neighbor `block_1` condition `%t7` and lhs `%t6` rows both have nonzero value
  ids and selected `branch_stack_slot` freshness at `block=3 inst=2`; the RHS
  differs at value-id identity admission, not at branch point freshness.
- A narrow removal of branch-stack zero-id guards would conflict with other
  prepared consumers that still reserve zero as a sentinel.

## Proof

Build proof: `cmake --build --preset default --target c4cll` passed with no
work to do.

Evidence artifacts:
- `build/agent_state/646_step1_same_block_identity/990127-1.bir.txt`
- `build/agent_state/646_step1_same_block_identity/990127-1.bir.err`
- `build/agent_state/646_step1_same_block_identity/990127-1.prepared-bir.txt`
- `build/agent_state/646_step1_same_block_identity/990127-1.prepared-bir.err`
- `build/agent_state/646_step1_same_block_identity/990127-1.case.log`
- `build/agent_state/646_step1_same_block_identity/990127-1.case.exitcode`
- `build/agent_state/646_step2_identity_boundary/identity_boundary_notes.md`

No root-level `.log` file was written for this read-only classification
packet.
