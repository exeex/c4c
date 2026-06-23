Status: Active
Source Idea Path: ideas/open/321_rv64_i16_local_array_select_store_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize I16 Local-Array Select/Store Evidence

# Current Packet

## Just Finished

Completed Step 1 fresh evidence normalization for `src/00143.c`.

Probe artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_321_step1/bir/src_00143.c.bir.txt`
- `build/rv64_c_testsuite_probe_latest/triage_321_step1/prepared/src_00143.c.prepared.txt`
- `build/rv64_c_testsuite_probe_latest/triage_321_step1/asm/src_00143.c.s`
- `build/rv64_c_testsuite_probe_latest/triage_321_step1/logs/`
- `build/rv64_c_testsuite_probe_latest/triage_321_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_321_step1/summary.md`

Probe result:

- BIR dump: exit 0.
- Prepared-BIR dump: exit 0.
- RV64 asm emit: exit 0.
- clang link: exit 1 with undefined temporary symbol `.Lmain_block_2`.
- qemu: not run because link failed.

Classification:

- The stack-homed fused-compare loop branch has emitted before the owned
  failure: prepared-BIR records `branch_condition for.cond.1 kind=fused_compare
  ... true=block_1 false=block_2`, and emitted RV64 contains `blt ...,
  .Lmain_block_1` followed by `j .Lmain_block_2`.
- Emission then enters `.Lmain_block_1` and truncates after the initial local
  index loads, before the first i16 local-array select/store sequence
  `%t10`/`%t9.elt0`/`%t9.store0` is emitted. Because emission stops there, the
  later `.Lmain_block_2` label is never defined and clang fails the link.
- Prepared facts distinguish this from idea 320: the i16 local-array
  store-source records are `status=available` with
  `source_producer=select_materialization`, not
  `missing_destination_access`.
- Prepared facts also show the in-scope i16 mechanism: i16 temporaries such as
  `%t10`, `%t9.elt0`, and `%t9.store0` have stack-slot homes with `bank=none`;
  memory access records exist for size-2 load/store accesses, but RV64 emission
  does not materialize the i16 select/store path.

## Suggested Next

Add focused expected-repair backend coverage for an i16 local-array
select/store emission boundary that proves truncation before semantic halfword
load/select/store lowering, without depending on `src/00143.c`, `%t9.store0`,
fixed labels, or fixed stack offsets.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_1`, `.Lmain_block_2`,
  `%t9.store0`, fixed source spelling, or observed stack offsets.
- Do not claim progress by widening the focused fixture from i16/halfword
  storage to int storage unless that is explicitly recorded as
  reclassification rather than repair.
- The current clang-visible missing label is a consequence of RV64 emission
  truncating in the i16 local-array block, not evidence that idea 319's
  stack-homed fused-compare branch is still the first current blocker.
- Do not fold stack-homed fused compare control flow, nested select-chain
  store-source publication, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The probe
artifacts above provide the Step 1 classification evidence, and `test_after.log`
is the canonical proof log.
