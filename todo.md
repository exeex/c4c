Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Callee-Side Byval Home Access

# Current Packet

## Just Finished

Completed idea 318 Step 4 by reproving `src/00140.c` after the Step 3 byval
payload transport repair and narrowing the remaining callee-side mechanism.

Probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_318_step4/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_318_step4/summary.md`
- BIR, prepared-BIR, emitted RV64 assembly, clang, and qemu logs under the same
  triage directory

Probe result for `src/00140.c`:

- `--dump-bir`: 0
- `--dump-prepared-bir`: 0
- RV64 asm emit: 0
- clang link: 0
- qemu: 139 because emitted `f1` remains incomplete and falls through

Implemented repair within Step 4:

- RV64 prepared i8 pointer-value loads now use `load_pointer_value_base_register`
  so stack-homed byval pointer formals can drive byte-field copies.
- This moves `src/00140.c` past the previous callee byte-field byval copy
  truncation.

Current classification:

- Callee byval pointer home setup is no longer the first bad mechanism. Emitted
  `f1` stores incoming `a0` into `%p.f` with `sd a0, 0(sp)`, and i32/i8/ptr
  lanes now load through `ld t3, 0(sp)` before reading from the byval payload.
- The first remaining emitted-code truncation is the float/FPR aggregate byval
  lane:
  `%lv.param.p.f.aggregate.param.copy.24 = bir.load_local float %lv.param.p.f.24, addr %p.f+24`.
- Prepared-BIR places that value in FPR `ft0` and records a pointer-value
  memory access at offset 24, size 4, align 8. RV64 prepared local memory
  emission does not yet lower float/FPR pointer-value load/store lanes.
- Per the packet boundary, float/FPR byval aggregate lane transport was not
  repaired here and should split or be handled by the next focused packet.
- A later caller-side pointer argument materialization issue may remain for
  `a1` sourced from preserved `%lv.f`/`s1`, but emitted code does not reach that
  semantic use before the float-lane truncation.

Focused byval coverage remains positive:

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call`
- `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`

## Suggested Next

Split or execute a focused packet for RV64 byval aggregate float/FPR lane
lowering, then reprobe `src/00140.c` again to see whether the later caller
pointer-argument materialization issue becomes first bad.

## Watchouts

- Do not fold broad variadic ABI or unrelated aggregate ABI rewrites into the
  float/FPR lane packet.
- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack homes.
- `src/00140.c` currently emits and links even when `f1` is incomplete, so qemu
  status 139 is a symptom of fallthrough after truncation, not a complete
  runtime execution of the intended function.

## Proof

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
focused byval dump/codegen/runtime tests pass in the final proof log. Fresh log:
`test_after.log`.
