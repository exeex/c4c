Status: Active
Source Idea Path: ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Select-Chain Residual Evidence

# Current Packet

## Just Finished

Completed idea 317 Step 1 evidence normalization for `src/00046.c`. Captured
fresh BIR, prepared-BIR, RV64 assembly, clang link, and qemu outcome under
`build/rv64_c_testsuite_probe_latest/triage_317_step1/`.

Result: dump/emit/link all succeed; qemu exits `2`.

First bad mechanism: select-chain/short-circuit materialization at
`logic.end.23`, not aggregate-local storage. Prepared-BIR lowers the `&&`
short-circuit join to:

```text
%t31 = bir.select ne i32 %t16, 2, i32 %t27, 0
%t32 = bir.ne i32 %t31, 0
```

The emitted RV64 reaches `.Lmain_logic_end_23` after correctly loading
`v.b1`/`v.b2` from the stack, but then derives a nonzero selected value on the
false LHS path and returns `2`. Aggregate-local stores/reloads from the idea
314 repair remain past the first residual: the assembly stores `v.b1`/`v.c`
with `sw` at their stack homes and reloads them with `lw` before the failing
short-circuit select path.

## Suggested Next

Add focused expected-repair backend coverage for the Step 2 boundary:
short-circuit `&&` lowering where a false LHS must publish `0` through the
select-chain result, plus a nearby same-block compare-result select where the
true arm is a compare value and the false arm is `0`. Include a small
aggregate-field load guard only to prove aggregate stores/reloads stay intact;
do not copy `src/00046.c` or depend on fixed stack offsets.

## Watchouts

- Do not reopen aggregate-local subobject stores/loads repaired by idea 314
  based on this packet; fresh evidence shows they are not the first bad
  mechanism here.
- Do not special-case `src/00046.c`, fixed offsets, or one exact expression
  shape.
- The failing path is semantic: `v.b1 == 2` should skip RHS publication and make
  the lowered `&&` result `0`, but emitted code produces a nonzero value before
  branching to the return-2 block.

## Proof

Probe artifacts:
`build/rv64_c_testsuite_probe_latest/triage_317_step1/probe_results.tsv` and
`build/rv64_c_testsuite_probe_latest/triage_317_step1/summary.md`.

Ran delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The build succeeded, but
the backend subset still did not pass because the existing
`backend_riscv_prepared_edge_publication` test failed with "RISC-V prepared
module should emit a register edge move"; `test_after.log` is the proof log.
