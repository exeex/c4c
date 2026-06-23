Status: Active
Source Idea Path: ideas/open/322_rv64_empty_loop_exit_successor_emission.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Empty Successor Exit Emission

# Current Packet

## Just Finished

Completed Step 3 repair for idea 322, focused on reachable empty loop-exit
successor emission without testcase-shaped routing.

Implementation:

- RV64 pointer frame-slot address materializations whose prepared home is a
  computed pointer base plus offset with no assigned register are treated as
  producer-side no-ops; consumers materialize from prepared address facts.
- Pointer local stores now preserve existing register-homed pointer sources and
  only fall back to producer-side address materialization when no prepared
  pointer register exists.
- Stack-homed operands for signed div/rem now use the existing scalar
  register-materialization helper instead of failing before the generic stack
  load path.
- RV64 prepared local memory emission now handles prepared pointer-value i16
  loads and stores with native `lh`/`sh` through the materialized pointer base.

Focused coverage:

- `backend_codegen_route_riscv64_empty_loop_exit_successor` was converted from
  expected-repair to a positive emitted-code contract requiring the successor
  body, switch-case labels, halfword operations, `remw`, and a valid return
  path.
- `backend_rv64_runtime_riscv64_empty_loop_exit_successor` now proves the
  fixture links and exits under qemu with status 0.
- The existing `backend_dump_riscv64_empty_loop_exit_successor` prepared-fact
  coverage remains in place.

Result:

- The focused fixture now emits all reachable loop-exit successor and switch
  body blocks and returns normally under qemu.
- Existing i16 halfword and stack-homed fused compare focused tests remain
  green.
- Previously regressed nearby pointer-local route/runtime tests were rerun and
  pass after tightening pointer store materialization fallback.

## Suggested Next

Execute Step 4 reprobe for `src/00143.c` through BIR, prepared-BIR, RV64 emit,
link, and qemu to classify whether idea 322 is closure-ready or exposes a
later residual.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_2`, `_IO_stdin_used`, fixed
  block order, linked section layout, or observed instruction addresses.
- The repair exposed and covered generic helper gaps needed to emit the
  successor body: computed pointer address publication, stack-homed signed
  remainder operands, and pointer-value i16 load/store lowering.
- Keep idea 319 stack-homed fused compare, idea 320 nested store-source
  publication, and idea 321 i16 local-array select/store as separate routes
  unless Step 4 fresh evidence shows a regression.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(empty_loop_exit_successor|i16_local_array_select_store|stack_homed_fused_compare_missing_false_label)'`

Result: passed, 9/9 tests.

Regression spot-check after an initial broad-run regression:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(codegen_route|rv64_runtime)_riscv64_(prepared_local_array_base_pointer|prepared_local_array_subobject_pointer|prepared_local_array_pointer_step|prepared_local_array_i8_element_access|aggregate_local_self_pointer_chain|empty_loop_exit_successor)'`

Result: passed, 10/10 tests.

Delegated proof to run:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero only because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
canonical proof log is `test_after.log`.
