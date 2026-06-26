Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Divmod And Check Residual Ownership

# Current Packet

## Just Finished

Step 3 proof/classification completed. The fresh `src/divmod-1.c` RV64
allowlist probe still fails, but not with the Step 2 no-bank same-module `i16`
call-argument shape.

Fresh probe result:

- `build/agent_state/407_step3_divmod_probe.log` reports
  `RV64_C4C_OBJ_COMPILE_FAIL`.
- The only case diagnostic in
  `build/rv64_gcc_c_torture_backend/src_divmod-1.c/case.log` is
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.
- The case work directory contains only `case.log`, so there is no more
  specific local diagnostic artifact from the delegated proof.

Fresh prepared dump result:

- `build/agent_state/407_step3_dumps/divmod-1.prepared.txt` shows the former
  blocking frame-slot same-module `i16` call arguments with `value_bank=gpr`,
  `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`, `dest_bank=gpr`, and
  `missing_frame_slot_arg_publication=yes`.
- No frame-slot same-module `i16` call argument remains in the old
  `value_bank=gpr ... dest_bank=none` state.
- `call_arg_stack_to_stack` move records still appear in move bundles for those
  frame-slot sources, but the call-plan section now publishes the GPR
  destination facts needed by consumers.

Ownership classification: the remaining `src/divmod-1.c` failure is outside
407. The 407 producer-side prepared ABI publication acceptance condition is
met. The residual generic `unsupported_instruction_fragment` belongs back to
the broader RV64 object-route instruction-fragment lowering track, represented
by 395, unless the supervisor opens a narrower follow-up from that route.

## Suggested Next

Route to plan-owner/reviewer for Step 4 close-or-route handling. If execution
continues instead, the next packet should be under the RV64 object-route
instruction-fragment owner, not this 407 prepared call-argument publication
idea.

## Watchouts

- Do not patch RV64 `object_emission.cpp` as part of 407; this packet only
  classified the residual after prepared facts became target-consumable.
- The prepared dump still prints stale semantic BIR `sret(...)` suffixes in the
  top BIR text for the original source call representation, but the prepared
  call-plan facts are repaired and publish GPR destinations.
- The delegated proof preserves `test_after.log`; backend CTest remains green.

## Proof

Ran the supervisor-delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/407_step3_dumps && printf '%s\n' 'src/divmod-1.c' > build/agent_state/407_step3_divmod.allowlist.txt && { ALLOWLIST=build/agent_state/407_step3_divmod.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/407_step3_divmod_probe.log 2>&1 || true; } && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/407_step3_dumps/divmod-1.prepared.txt 2> build/agent_state/407_step3_dumps/divmod-1.prepared.err && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:|arg index=.*value_bank|dest_bank=none|missing_frame_slot_arg_publication|call_arg_stack_to_stack' build/agent_state/407_step3_divmod_probe.log build/agent_state/407_step3_dumps/divmod-1.prepared.txt build/agent_state/407_step3_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results: command completed with exit code 0. The RV64 allowlist probe recorded
one failing case, `src/divmod-1.c`, with `RV64_C4C_OBJ_COMPILE_FAIL` and
generic `unsupported_instruction_fragment`. The prepared dump confirmed the
same-module `i16` call-argument destination facts are now published. Backend
CTest completed and `test_after.log` is fresh.
