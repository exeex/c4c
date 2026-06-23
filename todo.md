Status: Active
Source Idea Path: ideas/open/321_rv64_i16_local_array_select_store_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair RV64 Halfword Body Emission

# Current Packet

## Just Finished

Completed Step 3/4 repair for RV64 i16 local-array select/store prepared
emission and halfword body lowering.

Focused fixture:

- `tests/backend/case/riscv64_i16_local_array_select_store.c`

Focused tests:

- `backend_dump_riscv64_i16_local_array_select_store`
- `backend_codegen_route_riscv64_i16_local_array_select_store`
- `backend_rv64_runtime_riscv64_i16_local_array_select_store`

Repair result:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp` now accepts
  prepared size-2 i16 frame-slot accesses and emits native RV64 `lh`/`sh` for
  local halfword load/store publication.
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp` now materializes
  i16 values from stack homes, stores i16 cast/select results into size-2 stack
  homes, and lowers same-block i16 select chains without widening the local
  array path to i32.
- The focused codegen route test was flipped from expected-repair to a positive
  halfword contract requiring `lh`/`sh`, select labels, return publication, and
  no `lhu`. Runtime coverage now passes under qemu.
- Reprobe artifacts for `src/00143.c` are under
  `build/rv64_c_testsuite_probe_latest/triage_321_step3/`. The case now emits
  and links with semantic halfword select/store bodies. qemu still exits 132
  because `.Lmain_block_2` is an empty loop-exit successor at the end of emitted
  text and resolves to the next section (`_IO_stdin_used`) in the linked binary.
  That is a separate control-flow/empty-exit residual, not the i16 halfword
  select/store publication boundary.

## Suggested Next

Split or route the remaining `src/00143.c` failure as a separate RV64
control-flow/empty-exit emission residual: loop false successor labels must
own a valid body/return path instead of falling through into the next section.

## Watchouts

- The remaining `src/00143.c` qemu failure should not be folded back into idea
  321 unless the plan owner explicitly routes empty-exit/control-flow emission
  here. The repaired halfword path is already visible in the emitted candidate
  assembly before the loop-exit trap.
- Keep avoiding special-cases for `src/00143.c`, `.Lmain_block_2`,
  `%t9.store0`, fixed source spelling, or observed stack offsets.
- The focused route repaired stack-slot `bank=none` i16 values and size-2
  prepared memory accesses; destination-access publication and stack-homed
  fused compare control flow remain separate routes.

## Proof

Focused proof passed:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_i16_local_array_select_store'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero only because existing
unrelated `backend_riscv_prepared_edge_publication` still fails. The focused
`backend_dump_riscv64_i16_local_array_select_store`,
`backend_codegen_route_riscv64_i16_local_array_select_store`, and
`backend_rv64_runtime_riscv64_i16_local_array_select_store` tests all passed in
`test_after.log`.
