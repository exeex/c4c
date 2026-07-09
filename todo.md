Status: Active
Source Idea Path: ideas/open/650_edge_store_local_aggregate_publication_ordering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Representative Integration Proof

# Current Packet

## Just Finished

Completed Step 4 representative integration proof for the existing RV64
`edge_store_slot` carrier implementation.

Proof captured:

- Focused CTest subset passed 2/2:
  `backend_dump_riscv64_edge_store_local_publication_ordering` and
  `backend_cli_riscv64_edge_store_local_publication_ordering`.
- Representative object emission succeeded for
  `tests/c/external/gcc_torture/src/pr68185.c`:
  `build/agent_state/650_step4_representative_integration/pr68185.o`.
- Representative object emission succeeded for
  `tests/c/external/gcc_torture/src/pr68321.c`:
  `build/agent_state/650_step4_representative_integration/pr68321.o`.
- Disassembly evidence was captured in
  `build/agent_state/650_step4_representative_integration/pr68185.objdump.txt`
  and
  `build/agent_state/650_step4_representative_integration/pr68321.objdump.txt`.
- The disassembly grep found the representative function labels and RV64
  control/data movement instructions, including `main`, `fn1`, `auipc`, `j`,
  `mv`, `li`, and `ret` matches across the two generated objects.

Downstream owner status: no new downstream owner was exposed by these two
representatives after object emission advanced; both representatives compile to
RV64 objects and disassemble successfully.

## Suggested Next

Proceed to Step 5 broader RV64 validation. Use the supervisor-selected matched
regression guard or equivalent broader proof to verify that the Step 3 object
route admission did not regress nearby RV64 backend coverage.

## Watchouts

- Step 4 is evidence-only; it did not touch implementation files or test
  definitions.
- Broader validation should keep watching the out-of-SSA `loop_carry` move
  reason admission and the fail-closed `edge_store_slot` carrier constraints
  from Step 3.
- The focused representative proof shows object/disassembly progress for
  `pr68185.c` and `pr68321.c`; it does not replace the planned broader RV64
  regression check.

## Proof

Passed:

```sh
bash -lc 'set -o pipefail; mkdir -p build/agent_state/650_step4_representative_integration && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|cli)_riscv64_edge_store_local_publication_ordering$" && build/c4cll --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr68185.c -o build/agent_state/650_step4_representative_integration/pr68185.o && build/c4cll --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr68321.c -o build/agent_state/650_step4_representative_integration/pr68321.o && llvm-objdump -d build/agent_state/650_step4_representative_integration/pr68185.o > build/agent_state/650_step4_representative_integration/pr68185.objdump.txt && llvm-objdump -d build/agent_state/650_step4_representative_integration/pr68321.o > build/agent_state/650_step4_representative_integration/pr68321.objdump.txt && rg -n "<main>|<fn1>|ret|auipc|j\\s|mv|li" build/agent_state/650_step4_representative_integration/pr68185.objdump.txt build/agent_state/650_step4_representative_integration/pr68321.objdump.txt; } 2>&1 | tee test_after.log'
```

`test_after.log` contains the canonical proof log. The focused CTest subset
passed 2/2, both representative object-emission commands exited 0, both
`llvm-objdump` captures were written, and the delegated disassembly grep
returned matches.
