Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Callee-Side Byval Home Access

# Current Packet

## Just Finished

Added focused Step 4 backend coverage for the RV64 prepared local-memory
float/FPR pointer-value lane repaired in the previous packet.

New focused fixture:

- `tests/backend/case/riscv64_byval_aggregate_float_lane.c`

New focused tests:

- `backend_dump_riscv64_byval_aggregate_float_lane`
- `backend_codegen_route_riscv64_byval_aggregate_float_lane`

Coverage contract:

- The dump test proves a fixed-arity byval aggregate formal with an `F32` field
  lowers to prepared pointer-value `float` load/store records and FPR homes.
- The codegen test proves RV64 emission for that callee includes `flw`/`fsw`
  through the byval pointer-value base and completes the callee body.
- The fixture is callee-focused and does not copy `src/00140.c`, `%p.f`, fixed
  stack homes, or candidate-specific names.

Current classification:

- Callee byval pointer home setup is no longer the first bad mechanism. Emitted
  `f1` stores incoming `a0` into `%p.f` with `sd a0, 0(sp)`, and i32/i8/ptr
  lanes now load through `ld t3, 0(sp)` before reading from the byval payload.
- The float/FPR byval aggregate lane is no longer the first bad mechanism.
- The next first-bad mechanism is separate caller pointer argument
  materialization for frame-slot address arguments after byval payload staging.
  Prepared call plans select `%lv.f` from prior preservation in `s1`
  (`arg1 bank=gpr from=register:s1 to=a1`, and later args 4/5 also use `s1`),
  while emitted `main` passes `mv a1, s1`, `mv a4, s1`, and `mv a5, s1`
  without materializing `s1` as `%lv.f`'s frame-slot address.
- Emitted `f1` reaches `lw t3, 0(t0)` after the byval copy lanes and qemu
  segfaults. Per the packet boundary, caller pointer argument materialization is
  recorded as the next residual rather than repaired here.

Focused byval coverage remains positive:

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call`
- `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`

## Suggested Next

Split or execute a focused packet for RV64 caller pointer argument
materialization when a frame-slot address argument is selected from preservation
after byval payload staging.

## Watchouts

- Do not fold broad variadic ABI or unrelated aggregate ABI rewrites into the
  caller pointer argument materialization packet.
- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack homes.
- `src/00140.c` now emits a complete `f1` body through the byval copy lanes, but
  qemu status 139 still blocks semantic pass because pointer arguments sourced
  from `%lv.f` are not materialized correctly.
- Positive runtime coverage was not added for the new float-lane fixture because
  a linked binary containing the isolated `flw`/`fsw` lane exits under qemu with
  illegal instruction in this environment. The existing integer/pointer byval
  runtime fixture remains positive.

## Proof

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
new float-lane dump/codegen tests and existing fixed-call byval dump/codegen/
runtime tests pass in the final proof log. Fresh log: `test_after.log`.
