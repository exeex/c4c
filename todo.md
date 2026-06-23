Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reprobe And Close Classification

# Current Packet

## Just Finished

Completed the focused Step 5 RV64 caller pointer argument materialization repair
exposed after byval payload staging.

Implemented repair:

- RV64 prepared call emission now checks prepared frame-slot address
  materialization facts before falling back to a preserved source register for
  GPR call arguments.
- The materialization path resolves the prepared frame-slot stack offset and
  adds the active byval stack adjustment, so pointer arguments after temporary
  byval payload staging point back to the source local.
- Duplicate identical frame-slot materialization records for the same call
  instruction are accepted; conflicting materialization offsets still fail the
  fast path.

New focused fixture:

- `tests/backend/case/riscv64_byval_preserved_pointer_args.c`

New focused tests:

- `backend_dump_riscv64_byval_preserved_pointer_args`
- `backend_codegen_route_riscv64_byval_preserved_pointer_args`
- `backend_rv64_runtime_riscv64_byval_preserved_pointer_args`

Focused coverage result:

- The dump test proves a repeated byval call where the second call's byval
  aggregate and pointer argument are selected from prior preservation.
- The codegen test rejects stale `mv a1, s1` and proves active-stack-adjusted
  frame-slot address materialization after byval staging.
- The runtime test passes under qemu for the focused case.
- Existing byval fixed-call dump/codegen/runtime tests remain positive.
- Existing byval float-lane dump/codegen tests remain positive.

`src/00140.c` reprobe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_318_step5/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_318_step5/summary.md`
- BIR, prepared-BIR, emitted RV64 assembly, clang, and qemu logs under the same
  triage directory

`src/00140.c` status after this repair:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 139

Current classification:

- The caller pointer argument materialization residual is repaired. Emitted
  `main` now uses `addi a1, sp, 32`, `addi a4, sp, 32`, and `addi a5, sp, 32`
  after byval payload staging instead of stale preserved-register moves from
  `s1`.
- The next first-bad mechanism is separate callee incoming formal publication
  for non-byval GPR arguments in this prepared route. Emitted `f1` reads `%p.p`
  through `t0` (`lw t3, 0(t0)`) and `%p.n` through `s1`, but the function entry
  does not publish incoming `a1` to `t0` or incoming `a2` to `s1`.
- Per packet boundary, the callee formal-home residual was recorded and not
  folded into this caller materialization repair.

## Suggested Next

Split or execute a focused packet for RV64 prepared callee incoming formal
publication for non-byval GPR parameters when the prepared home is not already
the ABI argument register.

## Watchouts

- Do not fold broad variadic ABI, runtime F-extension behavior, or unrelated
  aggregate ABI rewrites into the callee formal-home packet.
- Do not special-case `src/00140.c`, `%p.f`, `%lv.f`, or fixed stack offsets.
- The byval pointer home, i8 lane, float/FPR lane, and caller pointer argument
  materialization boundaries now have focused positive coverage.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_byval_(aggregate_fixed_call|aggregate_float_lane|preserved_pointer_args)'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The new
preserved-pointer byval tests, existing fixed-call byval tests, and existing
float-lane byval tests pass in the final proof log. Fresh log:
`test_after.log`.
