Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 follow-up of `plan.md`: traced the remaining `00204.c`
runtime mismatch across the prepared-call to AArch64 machine-materialization
boundary without changing implementation or tests.

Last known-good facts after commit `5d0164280`:

- Semantic BIR for `arg` still has the expected `.global.aggregate.load.`
  lanes from globals into byval temporaries before calls such as `fa_s1`;
  e.g. `%t2.global.aggregate.load.0 = bir.load_local i8 %t2.0, addr s1`
  followed by `bir.call void fa_s1(ptr byval(size=1, align=1) %t2)`.
- Prepared BIR still records direct byval callsites with ABI destinations; for
  `fa_s1`, `prepared-call-plans` has `arg0 bank=gpr from=register:x21 to=x0`,
  and the corresponding before-call move has reason
  `call_arg_byval_aggregate_register_lanes`.
- For direct sret callers, prepared BIR records the hidden return pointer
  correctly. In `ret`, `fr_s1` has `memory_return=%t0`, `arg0
  bank=aggregate_address from=frame_slot:stack+1168 to=x8`, and final assembly
  materializes `add x8, sp, #153` before `bl fr_s1`.

First wrong machine/materialization facts:

- In final AArch64 for `arg`, after `printf("Arguments:\n")`, `x21` is never
  loaded with the `s1` byte. The code instead does `mov x13, x0`, stores
  stale `w13` to `[sp, #928]`, reloads it into `w0`, and calls `fa_s1`.
  Larger direct byval calls repeat the same stale scratch pattern.
- In final AArch64 for `fr_s1`, the callee receives the correct sret address
  in `x8` but stores stale `w13` to `[x8]`; it does not materialize the
  prepared `addr s1` load before the sret copy.
- Direct HFA arguments and HFA returns show the same class of failure for FPR
  lanes: prepared homes exist, but AArch64 materialization reuses scratch/home
  state instead of loading the aggregate payload lanes into the ABI registers.

Likely owned repair layer:

- Direct byval argument repair belongs in AArch64 call-boundary move emission
  and source selection, centered on
  `src/backend/mir/aarch64/codegen/calls.cpp`
  (`make_selected_call_argument_source`, `make_call_boundary_move_instruction`,
  and `materialize_missing_frame_slot_call_arguments`). The prepared producer
  in `src/backend/prealloc/regalloc/call_moves.cpp` publishes the byval
  register-lane move, but the AArch64 consumer materializes the aggregate
  carrier from stale storage instead of the payload lane source.
- Direct sret return repair belongs in AArch64 return/store materialization,
  centered on `src/backend/mir/aarch64/codegen/returns.cpp`
  (`lower_prepared_return_terminator` through `make_return_record` and
  `make_return_machine_instruction`) plus any shared memory-store source
  helper it calls. The caller-side hidden `x8` setup is good; the callee-side
  return copy source is wrong.

## Suggested Next

Repair direct AArch64 aggregate materialization before touching stdarg: teach
call-boundary move emission to materialize byval register lanes from their
prepared payload sources, and teach return/sret materialization to load the
direct aggregate return payload before storing through the hidden sret pointer.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Do not start with stdarg/HFA-vararg repair: the current signature proves
  direct aggregate byval arguments and direct aggregate/sret returns are already
  wrong before the variadic sections execute. Stdarg and HFA-varargs remain
  downstream until direct byval register lanes and direct aggregate/HFA returns
  are correct.
- Do not reinterpret the prepared `arg0 bank=gpr from=register:x21 to=x0`
  carrier as sufficient final payload proof; final assembly must show the
  global/stack payload bytes or FPR lanes being loaded into ABI registers.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8. The focused contract and both guard cases passed; 4 tests
ran, 3 passed, and `c_testsuite_aarch64_backend_src_00204_c` remains the only
failing test in this scope with a runtime mismatch. Canonical executor proof
log: `test_after.log`.
