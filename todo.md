Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate The Caller-Side Publication Gap

# Current Packet

## Just Finished

Step 1 classification completed. The prepared `i8` same-module call publishes
`value_bank=gpr`, `source_encoding=register`, `dest_bank=gpr`, and a before-call
`call_arg_register_to_register` move to `gpr:call_argument#0/w1 reg=a0`. The
prepared `i16` same-module call instead has the source value stack-homed and
publishes `value_bank=none`, `source_encoding=frame_slot`, `dest_bank=none`,
with a before-call `call_arg_stack_to_stack` move, even though a separate
register `abi_binding` for `a0` is present.

Producer trace: `src/backend/prealloc/legalize.cpp`
`direct_bir_call_arg_abi_repair()` already includes `I16` after closed 403, and
it feeds missing `CallInst::arg_abi` entries. The first caller-side gap is in
`src/backend/prealloc/call_plans.cpp`: `register_bank_from_type()` omits
`bir::TypeKind::I16`, so `register_bank_from_arg_abi()` maps integer `I16`
call arguments to `PreparedRegisterBank::None`. That leaves
`populate_call_plans()` / `plan_call_argument_destination()` unable to publish
the GPR destination bank for same-module `i16` call arguments, and it also
prevents `find_prepared_missing_frame_slot_call_argument_publication_need()` in
`src/backend/prealloc/calls.hpp` from recognizing the frame-slot-to-GPR
publication need.

## Suggested Next

First implementation packet: extend the scalar bank helper family in
`src/backend/prealloc/call_plans.cpp`, starting with
`register_bank_from_type()` so `bir::TypeKind::I16` maps to
`PreparedRegisterBank::Gpr`. Then regenerate/check the prepared `i16`
same-module call shape to confirm `value_bank`/`dest_bank` become GPR and the
existing `FrameSlotValue` path can drive RV64 frame-slot-to-register argument
loading without adding ABI inference to `object_emission.cpp`.

## Watchouts

- Do not patch RV64 `object_emission.cpp` to infer the missing scalar argument
  destination.
- Keep closed 403 formal ABI publication separate unless fresh facts show a
  regression in incoming formal handling.
- The before-call move producer in `src/backend/prealloc/regalloc.cpp` /
  `src/backend/prealloc/regalloc/call_moves.cpp` already records both the stack
  move and the later register ABI binding in the `i16` dump; the first missing
  fact is the call-plan value/destination bank publication, not raw ABI repair.
- After `I16` is exposed as GPR in the call plan, verify whether the existing
  `FrameSlotValue` consumer path is enough; if not, the next gap is an adjacent
  prepared frame-slot-value call-argument publication/helper issue, still in
  producer-prepared facts rather than RV64 ABI reconstruction.
- Preserve `test_after.log` as the canonical executor proof log.

## Proof

Ran the supervisor-delegated proof command exactly:

```sh
cmake --build --preset default && mkdir -p build/agent_state/407_step1_dumps && printf '%s\n' 'src/divmod-1.c' > build/agent_state/407_step1_i16_call_arg.allowlist.txt && { ALLOWLIST=build/agent_state/407_step1_i16_call_arg.allowlist.txt STOP_ON_FAILURE=0 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/407_step1_i16_call_arg_probe.log 2>&1 || true; } && rg -n 'unsupported_instruction_fragment|unsupported_stack_frame|unsupported_local_memory_access|unsupported_global_data|RV64_BACKEND_RUNTIME_MISMATCH|unsupported RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:' build/agent_state/407_step1_i16_call_arg_probe.log && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results: build was up to date; the narrow `src/divmod-1.c` probe still fails
with `RV64_C4C_OBJ_COMPILE_FAIL` and `unsupported_instruction_fragment` in
`build/agent_state/407_step1_i16_call_arg_probe.log`; backend CTest passed
`326/326` in `test_after.log`.
