Status: Active
Source Idea Path: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit Scalar Frame-Slot Value Call Arguments

# Current Packet

## Just Finished

Executed Plan Step 2 by adding focused RV64 object lowering for scalar
frame-slot-value call arguments.

Implemented support for direct calls whose prepared call argument has
`source_encoding=frame_slot`, a GPR value bank, a known stack-slot value home,
matching frame-slot/source-offset facts, an encodable final stack offset, and a
GPR ABI destination register. The lowering emits a scalar stack load from the
prepared frame slot directly into the argument register before the call
relocation.

Focused backend tests now prove:

- positive same-module scalar GPR call argument lowering from slot `#2` offset
  `16` into `a0`
- fail-closed rejection for frame-slot address selections, missing slot/home
  facts, non-GPR destinations, unsupported argument width, bad alignment,
  out-of-frame offsets, and stack-frame out-of-range offsets

## Suggested Next

Execute Plan Step 3 by rerunning `src/20000121-1.c` through the RV64 gcc
torture backend runner with a temporary one-case allowlist. Store the runner
log under `build/agent_state/` and record whether the representative passes or
advances to a distinct residual owner.

## Watchouts

- Do not include frame-slot address call arguments; those belong to idea 372.
- Do not handle aggregate `va_arg`, byval homes, terminators, or source-shape
  shortcuts in this route.
- The new path intentionally loads the scalar payload value from a prepared
  stack-slot home; it does not materialize frame-slot addresses.
- Out-of-range stack frames are still rejected by the existing stack-frame
  admission gate before the call-argument helper runs.

## Proof

Delegated proof passed and wrote `test_after.log`.

Command run:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Result: `3/3` tests passed.
