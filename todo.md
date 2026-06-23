Status: Active
Source Idea Path: ideas/open/316_rv64_residual_pointer_array_select_flow.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Pointer/Array/Select Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for the selected idea-316
pointer/select boundary. Added semantic nearby cases under `tests/backend/case/`
and registered:

- `backend_dump_riscv64_pointer_to_pointer_local_address`
- `backend_codegen_route_riscv64_pointer_to_pointer_local_address_expected_repair`
- `backend_dump_riscv64_pointer_integer_select_chain`
- `backend_codegen_route_riscv64_pointer_integer_select_chain_expected_repair`

## Suggested Next

Repair the first covered boundary: make local pointer-to-pointer frame-address
materialization store the source local `p` address for `&p`, then repair
pointer/integer select-chain materialization/publication enough to flip the new
expected-repair route checks to positive semantic emission/runtime contracts.

## Watchouts

- Do not special-case candidate filenames, observed stack offsets, or source
  expression shapes.
- The pointer-to-pointer case is deliberately not a c-testsuite clone: it
  asserts `p = &value`, `pp = &p`, and `**pp = 7`; current RV64 emission
  truncates after loading the mis-materialized pointer slot.
- The pointer/integer select-chain case is deliberately small: it uses null
  pointer ternaries and local pointer publication; current RV64 emission reaches
  select materialization and then emits no-op moves before truncating.
- `src/00077.c` and `src/00143.c` still look like separate stack-homed
  compare/control-flow residuals before their later pointer/array bodies.
- Keep aggregate/byval and function-pointer repair out of this route.

## Proof

Ran `cmake --build --preset default -j && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log`. The four new
expected-repair tests passed. The delegated backend proof still did not pass
because `backend_riscv_prepared_edge_publication` failed with "RISC-V prepared
module should emit a register edge move"; `test_after.log` is the proof log.
