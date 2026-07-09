Status: Active
Source Idea Path: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Same-Block Identity Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` is complete: refreshed same-block branch identity evidence
for `tests/c/external/gcc_torture/src/990127-1.c`.

Current first owner remains the RV64 object-route branch stack-load authority
diagnostic:
`unsupported_branch_stack_load_authority`, `function=main`, `block=block_1`,
`block_index=3`, `terminator_instruction_index=2`, `role=rhs`,
`value=%lv.a`, `authority_status=home_value_mismatch`,
`source_freshness_status=missing_value`, `source_freshness_candidates=0`.

Semantic BIR shows the consumer shape at `block_1`: `%t6 = bir.load_local ptr
%lv.pa`, `%t7 = bir.eq ptr %t6, %lv.a`, then `bir.cond_br i1 %t7,
block_3, block_4`.

Prepared BIR preserves the same consumer point as `block_1` inst 2:
`bir.cond_br i32 %t7, block_3, block_4`. The prepared authority row for
`block_1 role=rhs value=%lv.a value_id=0` does not directly print a selected
home/slot/object on the mismatch row. Nearest visible prepared evidence is
`home %lv.a value_id=0 kind=stack_slot slot_id=18 offset=104`, storage
`%lv.a frame_slot ... spill_slot=slot#18+stack104 offset=104`, and
`address_materialization block=block_1 inst_index=1 kind=frame_slot
result=%lv.a offset=0`.

## Suggested Next

Step 2 should locate the explicit home/value identity authority boundary for
this row: either the prepared producer/publication path should expose an
identity fact connecting the branch RHS `%lv.a` address materialization to the
selected home, or the RV64 consumer lookup is using the wrong key and failing
to see an already-published fact. The current evidence points to a missing or
unconsumed same-block home/value identity fact, not to generic source
freshness reopening.

## Watchouts

- Do not infer value identity from stack offsets, final assembly, source
  syntax, local names, or diagnostics.
- Do not reopen prepared branch stack-source freshness publication from idea
  636.
- Do not absorb RV64 terminator-fragment work from the now-closed idea 645.
- The mismatch authority row omits selected-home fields, so Step 2 should not
  treat the visible `%lv.a` stack home as proof of branch RHS identity by
  itself.

## Proof

Build proof: `cmake --build --preset default --target c4cll` completed with
no work to do.

Evidence artifacts:
- `build/agent_state/646_step1_same_block_identity/990127-1.bir.txt`
- `build/agent_state/646_step1_same_block_identity/990127-1.bir.err`
- `build/agent_state/646_step1_same_block_identity/990127-1.prepared-bir.txt`
- `build/agent_state/646_step1_same_block_identity/990127-1.prepared-bir.err`
- `build/agent_state/646_step1_same_block_identity/990127-1.case.log`
- `build/agent_state/646_step1_same_block_identity/990127-1.case.exitcode`

Focused object-route refresh used the repository RV64 GCC torture backend
runner variables from `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
with `SRC` resolved to
`/workspaces/c4c/tests/c/external/gcc_torture/src/990127-1.c`; it exited 1
with the expected current compile failure above. No root-level `.log` file was
written for this diagnostics-only packet.
