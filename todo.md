Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair First Coherent Pointer/Array/Select Mechanism

# Current Packet

## Just Finished

Completed the remaining idea 316 Step 3 repair boundary for RV64
pointer/integer select-chain materialization and local pointer publication.
RV64 prepared scalar emission now materializes null pointer immediates,
`inttoptr`/`ptrtoint` register or stack destinations, and nested same-block I32
select producers that have no standalone prepared home. RV64 local pointer
stores/loads now also recover same-block prepared frame-slot accesses by
stored/result value name when select-chain expansion shifts the BIR instruction
index away from the memory-access record.

Converted the pointer/integer select-chain coverage from expected-repair to
positive semantic contracts:

- `backend_dump_riscv64_pointer_integer_select_chain`
- `backend_codegen_route_riscv64_pointer_integer_select_chain`
- `backend_rv64_runtime_riscv64_pointer_integer_select_chain`

## Suggested Next

Reprobe idea 316's candidate bucket, then decide whether the remaining
`src/00077.c` / `src/00143.c` stack-homed compare/control-flow residuals belong
in a follow-up packet or should split from this pointer/select route.

## Watchouts

- Do not special-case candidate filenames, observed stack offsets, or source
  expression shapes.
- The pointer-to-pointer case is deliberately not a c-testsuite clone: it
  asserts `p = &value`, `pp = &p`, and `**pp = 7`, and now passes qemu.
- The pointer/integer select-chain case is deliberately small and now passes
  qemu with real indirect select-chain materialization and frame-slot pointer
  publication; it remains separate from stack-homed compare/control-flow.
- `src/00077.c` and `src/00143.c` still look like separate stack-homed
  compare/control-flow residuals before their later pointer/array bodies.
- Keep aggregate/byval and function-pointer repair out of this route.

## Proof

Ran focused proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_pointer_integer_select_chain'`,
which passed 3/3 tests. Also manually emitted, linked, and ran
`tests/backend/case/riscv64_pointer_integer_select_chain.c` under qemu with
exit code 0. Then ran the delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The repaired
pointer/integer select-chain tests passed in the full backend run. The
delegated backend proof still did not pass because
`backend_riscv_prepared_edge_publication` failed with "RISC-V prepared module
should emit a register edge move"; `test_after.log` is the proof log.
