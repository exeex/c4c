Status: Active
Source Idea Path: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement One Semantic Identity Path

# Current Packet

## Just Finished

Step 3 of `plan.md` is complete: implemented the prepared value-id sentinel
repair by allocating real liveness/prepared value ids from `1` in
`BirPreAlloc::run_liveness`, leaving `PreparedValueId{0}` available as the
invalid sentinel already used by prepared publication, lookup, object traversal,
and RV64 freshness checks.

The representative same-block RHS `%lv.a` row now has a nonzero identity and
selected branch-stack freshness:
`home %lv.a value_id=1 kind=stack_slot slot_id=18 offset=104`, and
`branch_stack_load_authority function=main block=block_1 role=rhs value=%lv.a
value_id=1 ... status=available source_freshness_status=selected
source_freshness_candidates=1 source_freshness_authority=branch_stack_slot`.
This advances past the prior `home_value_mismatch` /
`source_freshness_status=no_candidate` owner without removing zero-id guards or
inferring identity from stack offsets, source names, final assembly, or
diagnostics.

Focused RV64 object-route residual owner for
`tests/c/external/gcc_torture/src/990127-1.c` is now
`unsupported_terminator_fragment: BIR terminator requires unsupported RV64
object lowering`.

## Suggested Next

Step 4 should decide whether the active idea is complete now that the
same-block home/value identity blocker is repaired, or whether to hand off a
separate plan for the residual RV64 `unsupported_terminator_fragment` owner.

## Watchouts

- The semantic repair shifts real prepared/liveness ids by `+1` for code that
  uses `BirPreAlloc::run_liveness`; the delegated focused proof did not require
  expected-snippet updates.
- The residual `unsupported_terminator_fragment` is outside this packet's
  value-id sentinel allocation owner. Do not fold terminator lowering into this
  completed slice without supervisor direction.
- Keep `PreparedValueId{0}` as the invalid sentinel; do not remove downstream
  fail-closed zero-id guards to make individual rows pass.

## Proof

Delegated proof command passed and is recorded in `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log
2>&1`.

Focused RV64 GCC torture object case for
`tests/c/external/gcc_torture/src/990127-1.c` was run through
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`; it exited
`1` with the new residual owner `unsupported_terminator_fragment`.

Evidence artifacts:
- `test_after.log`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.case.log`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.case.exitcode`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.prepared-bir.txt`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.prepared-bir.err`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.prepared-bir.exitcode`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.bir.txt`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.bir.err`
- `build/agent_state/646_step3_value_id_sentinel/990127-1.bir.exitcode`
