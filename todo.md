Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish Complete GPR Destinations For Frame-Slot I16 Arguments

# Current Packet

## Just Finished

Completed Step 2 / Publish Complete GPR Destinations For Frame-Slot I16
Arguments.

Implementation summary:

- `regalloc_detail::resolve_call_arg_abi(target_profile, call, arg_index)` now
  treats the call argument value type as the scalar `i16` authority and repairs
  or infers an integer-register ABI when raw call metadata is missing or still
  carries the stale stack/sret classification.
- `append_call_arg_move_resolution(...)` now uses the target-aware repaired ABI
  instead of raw call metadata, so call-argument moves and register placement
  calculations agree with `call_arg_storage_kind(...)` and
  `call_arg_abi_register_index(...)`.
- `append_prepared_call_abi_bindings(...)` and
  `append_call_arg_move_resolution(...)` normalize repaired scalar `i16`
  call-argument placements to `PreparedRegisterBank::Gpr` when the common
  placement constructor returns a bankless call-argument placement.
- The focused prepared contract test now covers the live frame-slot/no
  preexisting argument-ABI path by omitting synthetic call `arg_abi`, asserting
  `FrameSlotValue` source selection, checking the before-call move/binding
  destinations, and checking final call-plan GPR destination publication.

Fresh `divmod-1.c` prepared outcome:

- Same-module frame-slot `i16` calls to `div2`, `div4`, `mod2`, and `mod4` now
  publish `dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`,
  `dest_bank=gpr`, and `missing_frame_slot_arg_publication=yes`.
- Their before-call producer facts now use
  `destination_storage=register`, `placement=gpr:call_argument#N/w1`, and
  `reason=call_arg_stack_to_register`.
- The old blocking `dest_bank=none`, `placement=none:call_argument`, and
  `call_arg_stack_to_stack` shapes are absent from the Step 2 prepared dump.

## Suggested Next

Delegate Step 3 to run the representative RV64 GCC torture backend probe for
`src/divmod-1.c` and classify the new residual. If it still fails, record the
fresh diagnostic family and route it back to 395 only if the producer-side
`i16` call-argument facts remain complete.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Do not regress the repaired producer facts: Step 2 deliberately avoided a
  raw argument-index fallback in call-plans and repaired the existing
  target-aware call-ABI/register-index authority instead.
- The common target-register placement helper still has a bankless `I16`
  surface outside this packet's owned scope; the Step 2 producer sites
  normalize the repaired scalar `i16` placement to GPR locally.

## Proof

Command:

```sh
cmake --build --preset default && mkdir -p build/agent_state/407_reopen_step2_dumps && build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/407_reopen_step2_dumps/divmod-1.prepared.txt 2> build/agent_state/407_reopen_step2_dumps/divmod-1.prepared.err && rg -n 'arg index=.*value_bank|dest_bank=none|dest_placement=gpr:call_argument|missing_frame_slot_arg_publication|call_arg_stack_to_stack|i16 sret|abi_binding destination_kind=call_argument_abi' build/agent_state/407_reopen_step2_dumps/divmod-1.prepared.txt build/agent_state/407_reopen_step2_dumps/divmod-1.prepared.err && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Results:

- Build completed.
- Prepared dump written to
  `build/agent_state/407_reopen_step2_dumps/divmod-1.prepared.txt`;
  `.prepared.err` is empty.
- The proof `rg` confirmed complete frame-slot `i16` call-plan destinations
  and register call-ABI bindings. A follow-up check found no
  `dest_bank=none`, `call_arg_stack_to_stack`, or
  `placement=none:call_argument` matches in the Step 2 prepared dump.
- Backend CTest completed in `test_after.log`: `100% tests passed, 0 tests
  failed out of 326`.
