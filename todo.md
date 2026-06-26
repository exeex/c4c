Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Current Frame-Slot I16 Destination Gap

# Current Packet

## Just Finished

Completed Step 1 / Reconfirm Current Frame-Slot I16 Destination Gap.

Fresh `divmod-1.c` prepared facts still show the incomplete producer shape for
same-module `i16` frame-slot call arguments:

- `div2`: `arg index=0 value_bank=gpr source_encoding=frame_slot ... dest_bank=none`
  and before-call move `reason=call_arg_stack_to_stack`.
- `div4`: both `i16` arguments are `source_encoding=frame_slot` with
  `dest_bank=none`, and both moves use `reason=call_arg_stack_to_stack`.
- `mod2`: `arg index=0 value_bank=gpr source_encoding=frame_slot ... dest_bank=none`
  and before-call move `reason=call_arg_stack_to_stack`.
- `mod4`: both `i16` arguments are `source_encoding=frame_slot` with
  `dest_bank=none`, and both moves use `reason=call_arg_stack_to_stack`.

Contrasting supported facts are present in the same dump: register/immediate
same-module arguments publish `dest_placement=gpr:call_argument#N/w1`,
`dest_reg=aN`, and `dest_bank=gpr` for `div1`, `div3`, `mod1`, `mod3`,
`mod5`, `mod6`, and `exit`.

Producer trace:

- The first incomplete producer family is
  `src/backend/prealloc/regalloc/call_return_abi.cpp`:
  `resolve_call_arg_abi(...)`, `call_arg_abi_register_index(...)`, and
  `call_arg_storage_kind(...)`.
- That family feeds both call-ABI binding and call-argument move production:
  `append_prepared_call_abi_bindings(...)` in
  `src/backend/prealloc/regalloc.cpp` and
  `append_call_arg_move_resolution_records(...)` in
  `src/backend/prealloc/regalloc/call_moves.cpp`.
- `src/backend/prealloc/call_plans.cpp::plan_call_argument_destination(...)`
  is the final publication surface. It already tries to synthesize same-module
  `i16` GPR destinations through
  `regalloc_detail::call_arg_abi_register_index(...)`, but the live frame-slot
  shape arrives with stack-slot ABI bindings/moves, so the destination remains
  unpublished.

The current focused test did not catch the live shape because
`make_same_module_i16_call_argument_contract_module()` supplies explicit
synthetic `arg_abi` metadata and asserts only the final call-plan publication.
`divmod-1.c` exercises the frame-slot/no usable register-binding path where
regalloc produces stack-slot call-ABI bindings and `call_arg_stack_to_stack`
moves before call-plans run.

## Suggested Next

Delegate Step 2 to repair the producer-side ABI metadata/index authority for
same-module frame-slot `i16` call arguments. Start in
`regalloc_detail::resolve_call_arg_abi(...)`,
`regalloc_detail::call_arg_abi_register_index(...)`, and
`regalloc_detail::call_arg_storage_kind(...)`, then verify that
`append_prepared_call_abi_bindings(...)`,
`append_call_arg_move_resolution_records(...)`, and
`plan_call_argument_destination(...)` publish register call-argument
destinations for the live frame-slot shape without adding RV64 consumer-side
inference.

Update the focused prepared contract test so it covers the frame-slot source
selection/no preexisting register-binding shape, not just a synthetic call with
explicit argument ABI metadata.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- The live incomplete shape is partially repaired compared with the oldest
  407 notes: `value_bank=gpr` is present, but `dest_bank=none` and
  `call_arg_stack_to_stack` remain blocking.
- Avoid reintroducing a raw argument-index fallback in call-plans. The Step 2
  repair should make the existing call-ABI register-index authority return the
  usable register index for this same-module `i16` frame-slot shape.

## Proof

Command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/407_reopen_step1_dumps && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/407_reopen_step1_dumps/divmod-1.prepared.txt 2> build/agent_state/407_reopen_step1_dumps/divmod-1.prepared.err && rg -n 'arg index=.*value_bank|dest_bank=none|dest_placement=gpr:call_argument|missing_frame_slot_arg_publication|call_arg_stack_to_stack|i16 sret|abi_binding destination_kind=call_argument_abi' build/agent_state/407_reopen_step1_dumps/divmod-1.prepared.txt build/agent_state/407_reopen_step1_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results:

- Build completed with no work to do.
- Prepared dump written to
  `build/agent_state/407_reopen_step1_dumps/divmod-1.prepared.txt`;
  `.prepared.err` is empty.
- The proof `rg` confirmed both the incomplete frame-slot `i16`
  `dest_bank=none`/`call_arg_stack_to_stack` facts and the contrasting
  supported `dest_placement=gpr:call_argument` facts.
- Backend CTest completed in `test_after.log`: `100% tests passed, 0 tests
  failed out of 326`.
