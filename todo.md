Status: Active
Source Idea Path: ideas/open/646_rv64_branch_same_block_home_value_identity_reconciliation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Boundaries And Residual Owners

# Current Packet

## Just Finished

Step 4 of `plan.md` is complete: reran the delegated RV64 object-emission proof
and focused `990127-1.c` boundary validation after the Step 3 prepared value-id
sentinel repair.

The representative same-block RHS `%lv.a` identity row remains selected in the
fresh prepared dump:
`branch_stack_load_authority function=main block=block_1 role=rhs value=%lv.a
value_id=1 ... status=available source_freshness_status=selected
source_freshness_candidates=1 source_freshness_authority=branch_stack_slot`.
The dump also retains `home %lv.a value_id=1 kind=stack_slot slot_id=18
offset=104`; no `home_value_mismatch` regression signature was found in the
fresh prepared dump.

The focused RV64 object-route case for
`tests/c/external/gcc_torture/src/990127-1.c` still exits `1`, now with the
separate residual owner
`unsupported_terminator_fragment: BIR terminator requires unsupported RV64
object lowering`. This is outside the active same-block home/value identity
owner and was not repaired in this packet.

## Suggested Next

Supervisor should decide whether to close or retire this active runbook now
that the same-block identity boundary is validated, or open a separate
initiative for the RV64 `unsupported_terminator_fragment` owner.

## Watchouts

- `backend_riscv_object_emission` passed with its existing negative coverage,
  including branch-authority status rejection, fused-compare branch fail-closed
  shapes, pointer-stack branch authority rejection, move-bundle fail-closed
  shapes, and prepared pointer-add/freshness rejection paths.
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
`1` with the residual owner `unsupported_terminator_fragment`.

Evidence artifacts:
- `test_after.log`
- `build/agent_state/646_step4_boundary_validation/990127-1.case.log`
- `build/agent_state/646_step4_boundary_validation/990127-1.case.exitcode`
- `build/agent_state/646_step4_boundary_validation/990127-1.prepared-bir.txt`
- `build/agent_state/646_step4_boundary_validation/990127-1.prepared-bir.err`
- `build/agent_state/646_step4_boundary_validation/990127-1.prepared-bir.exitcode`
- `build/agent_state/646_step4_boundary_validation/990127-1.bir.txt`
- `build/agent_state/646_step4_boundary_validation/990127-1.bir.err`
- `build/agent_state/646_step4_boundary_validation/990127-1.bir.exitcode`
