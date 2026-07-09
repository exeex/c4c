Status: Active
Source Idea Path: ideas/open/634_large_selected_pointer_offset_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 Large-Offset Consumer Admission

# Current Packet

## Just Finished

Step 4 of idea `634` admitted narrow RV64 large selected pointer-value local-memory rows through explicit producer-published `rv64_large_selected_pointer_offset_scratch_clobber_authority`.

The stack-layout pointer-indirect address producer now publishes the authority for RV64 rows whose prepared pointer-value address facts already match the large-offset contract: pointer-value base, base-plus-offset addressing, non-12-bit byte offset, nonzero compatible width/alignment, and requested-range facts matching the access. The consumer contract returns `Available` only after those row facts are present, the authority bit is set, and the reserved large-offset scratch register `t6` is not itself the pointer base or any prepared GPR value home. Missing authority remains `MissingScratchClobberAuthority`; malformed occupied-`t6` authority returns `MalformedScratchClobberAuthority` and the object route still rejects through the generic local-memory diagnostic.

Authorized rows materialize the non-12-bit selected pointer offset with the existing prepared immediate helper into `t6`, add the pointer base into `t6`, and consume the resulting address with offset zero. Narrow immediate rows still use `prepared_pointer_value_base_offset`, and frame-slot/byval/sret/global/string local-memory behavior remains on its prior paths.

## Suggested Next

Supervisor should review this producer-plus-consumer Step 4 slice for acceptance and decide whether the current plan is ready for closure review or another focused packet.

## Watchouts

The authority bit defaults false and is published only by the RV64 stack-layout pointer-indirect producer path; handwritten or malformed prepared rows remain fail-closed unless they carry coherent authority and pass the consumer's `t6` occupancy check. No external expectations or allowlists were touched.

## Proof

Ran the delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

`test_after.log` shows `backend_riscv_object_emission` passed, 1/1 tests, 0 failures.
