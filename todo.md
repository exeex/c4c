Status: Active
Source Idea Path: ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Repair Or Prove The Object Contract

# Current Packet

## Just Finished

Step 2: Repair Or Prove The Object Contract. Repaired the RV64 object-route
`LocalFrameAddressMaterialization` call-argument consumer in
`fragment_for_prepared_call` so it passes the ABI destination register directly
to `append_rv64_prepared_local_frame_address_call_argument_source` when the call
argument has a GPR destination. The focused object route now emits direct
`mv a0, sp`/`addi a0, sp, 0` materialization instead of the previous
`mv s1, sp; mv a0, s1` two-step path. Summary:
`build/agent_state/677_step2_materialization_repair/summary.md`.

## Suggested Next

Supervisor should review and commit the completed Step 2 code plus canonical
execution artifacts, or choose the next packet from `plan.md` if broader
validation is desired.

## Watchouts

- The repair is semantic and general for object-route GPR call arguments with
  `LocalFrameAddressMaterialization`; no tests, byte contracts, unsupported
  markers, allowlists, runtime policy, or baseline accounting were changed.
- Do not repair or reclassify the pointer/global-local publication row; idea
  676 is closed.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'backend_cli_riscv64_call_arg_local_frame_address_materialization|backend_codegen_route_riscv64_call_arg_local_frame_address_materialization|backend_dump_riscv64_call_arg_local_frame_address_materialization'`.
Build succeeded and all three focused tests passed. Proof log:
`test_after.log`.
