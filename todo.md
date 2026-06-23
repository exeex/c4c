Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair First Coherent Pointer/Array/Select Mechanism

# Current Packet

## Just Finished

Completed Step 3's first repair boundary for RV64 local pointer-to-pointer
frame-address materialization. Frame-slot address materialization now uses the
prepared absolute stack offset, so `&p` materializes as the source local pointer
slot rather than the destination pointer slot. RV64 pointer-value local memory
emission also now handles the required stack-homed pointer load/base path for
the focused `**pp` store.

Converted the pointer-to-pointer coverage from expected-repair to positive
contracts:

- `backend_dump_riscv64_pointer_to_pointer_local_address`
- `backend_codegen_route_riscv64_pointer_to_pointer_local_address`
- `backend_rv64_runtime_riscv64_pointer_to_pointer_local_address`

## Suggested Next

Repair the remaining selected Step 2 mechanism:
`riscv64_pointer_integer_select_chain` still has expected-repair dump/emission
coverage and should be fixed without folding in the separate `src/00077.c` /
`src/00143.c` stack-homed compare residuals.

## Watchouts

- Do not special-case candidate filenames, observed stack offsets, or source
  expression shapes.
- The pointer-to-pointer case is deliberately not a c-testsuite clone: it
  asserts `p = &value`, `pp = &p`, and `**pp = 7`, and now passes qemu.
- The pointer/integer select-chain case is deliberately small: it uses null
  pointer ternaries and local pointer publication; current RV64 emission reaches
  select materialization and then emits no-op moves before truncating.
- `src/00077.c` and `src/00143.c` still look like separate stack-homed
  compare/control-flow residuals before their later pointer/array bodies.
- Keep aggregate/byval and function-pointer repair out of this route.

## Proof

Ran focused proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_pointer_to_pointer_local_address'`,
which passed 3/3 tests. Then ran the delegated proof
`cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The repaired
pointer-to-pointer tests passed in the full backend run. The delegated backend
proof still did not pass because `backend_riscv_prepared_edge_publication`
failed with "RISC-V prepared module should emit a register edge move";
`test_after.log` is the proof log.
