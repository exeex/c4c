Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Publish Same-Module I16 Argument ABI Facts

# Current Packet

## Just Finished

Step 2 producer repair completed. Same-module scalar `I16` call arguments now
publish GPR value and destination facts without a parallel raw argument-index
fallback.

Concrete changes:

- `src/backend/prealloc/call_plans.cpp` maps `I16` to
  `PreparedRegisterBank::Gpr`, uses `call_argument_value_bank()` for argument
  value-bank publication, and only synthesizes same-module `I16` destinations
  through the existing `regalloc_detail::call_arg_abi_register_index(...)`
  authority.
- `src/backend/prealloc/legalize.cpp` repairs impossible existing scalar `I16`
  call-argument ABI metadata, such as stale `sret` / stack / memory-class
  metadata on scalar arguments, back to direct integer register ABI metadata.
- `src/backend/prealloc/regalloc/call_return_abi.cpp` makes the target-aware
  call-argument ABI resolver and register-index computation repair/read that
  scalar `I16` ABI authority consistently.
- The focused prepared contract test now uses malformed scalar `I16` ABI
  metadata on a same-module call and asserts the frame-slot source shape
  publishes GPR destination facts and the missing-frame-slot argument
  publication query.

Fresh representative prepared dump:
`build/agent_state/407_step2_dumps/divmod-1.prepared.txt` now shows the
frame-slot same-module `i16` call arguments as `value_bank=gpr` with
`dest_placement=gpr:call_argument#N/w1`, `dest_reg=aN`, `dest_bank=gpr`, and
`missing_frame_slot_arg_publication=yes`. The previous
`value_bank=gpr ... dest_bank=none` frame-slot `i16` shape is gone.

## Suggested Next

Next packet should classify the remaining `src/divmod-1.c` RV64 object-route
failure after the prepared producer now publishes target-consumable `i16`
same-module call-argument destinations. Keep that classification outside RV64
object-emission ABI inference unless the supervisor explicitly changes scope.

## Watchouts

- Do not reintroduce a parallel raw argument-index fallback; same-module `I16`
  destination publication is now routed through
  `regalloc_detail::call_arg_abi_register_index(...)`.
- `call_arg_stack_to_stack` move records still appear in the move bundle for
  the representative frame-slot sources, but the call plan now carries the GPR
  destination facts and missing-frame-slot argument publication facts needed by
  consumers.
- The prepared dump still prints the original semantic BIR call text with stale
  `sret(...)` suffixes in the top BIR section; the repaired producer facts are
  visible in the call-plan section.
- `test_after.log` is fresh and contains the backend CTest proof.

## Proof

Ran the supervisor-delegated proof command exactly:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Results: build passed; backend CTest passed `326/326`. Re-ran the same backend
CTest subset with output redirected to preserve `test_after.log`, also exit
code 0.

Prepared dump command:

```sh
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/divmod-1.c > build/agent_state/407_step2_dumps/divmod-1.prepared.txt 2> build/agent_state/407_step2_dumps/divmod-1.prepared.err
```

Result: dump succeeded. The frame-slot same-module `i16` call arguments now
publish GPR destination facts through existing ABI/register-index authority.
