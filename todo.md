Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Iterate Or Split Remaining Candidate Buckets

# Current Packet

## Just Finished

Repaired the focused RV64 pointer-typed select materialization/publication
boundary for prepared emission. Pointer `select` results now lower through the
same branch-shaped select emission used for scalar selects, with pointer arms
materialized into pointer registers/homes. Local-frame address arms are resolved
through prepared stack-object/frame-slot metadata before generic register moves,
so `ptr %lv.value` becomes an `sp`-relative address instead of a no-op self
move. Pointer local publication stores/reloads use 8-byte `sd`/`ld` paths.

Converted the focused coverage to positive contracts:

- `backend_dump_riscv64_pointer_typed_select_publication`
- `backend_codegen_route_riscv64_pointer_typed_select_publication`
- `backend_rv64_runtime_riscv64_pointer_typed_select_publication`

The existing positive pointer-to-pointer local-address and pointer/integer
select-chain fixtures stayed positive.

## Suggested Next

Run the formal Step 4/Step 5 candidate reprobe for `src/00144.c`. A quick
post-repair emit/link/qemu check now emits and links, but qemu exits 132 after
the emitted assembly reaches a later truncated tail with repeated `mv t0, t0`
and no `ret`; that looks like a remaining select-chain/publication residual
rather than the focused local pointer publication case repaired here.

## Watchouts

- Do not weaken the existing positive `riscv64_pointer_integer_select_chain`
  tests; the repaired fixture covers a different `ptr` select boundary.
- The positive codegen test checks semantic instruction shapes without naming
  `src/00144.c` or depending on one fixed frame offset.
- `src/00077.c` and `src/00143.c` remain classified as separate stack-homed
  compare/control-flow residuals from the prior Step 4 reprobe.

## Proof

Ran focused proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_pointer_(typed_select_publication|integer_select_chain|to_pointer_local_address)'`,
which passed 9/9 tests.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The repaired pointer
tests passed in the full backend run. The backend subset still did not pass
because the existing `backend_riscv_prepared_edge_publication` test failed with
"RISC-V prepared module should emit a register edge move"; `test_after.log` is
the proof log.
