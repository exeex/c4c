Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Import structured values into generic BIR SSA edges

# Current Packet

## Just Finished

- Completed Plan Step 3: structured LIR inline-asm inputs/results now map
  transactionally through a per-function ordinary BIR value map, including
  chained and read/write edges, while only original semantic payload fields
  enter `InlineAsmNode`.

## Suggested Next

- Execute Plan Step 4, “Prove the completed structured transport,” using the
  supervisor-selected focused or broader proof without claiming general
  LIR-to-BIR support.

## Watchouts

- Step 4 proves only the bounded producer/printer/importer transport route; it
  does not authorize MIR-ready admission, target preparation, register
  allocation, spill/reload, or MIR architecture work.
- Preserve `original_asm_text` and `original_constraint_text` as the only BIR
  payload authority, and preserve rejection of textual `args_str` fallback
  while interpreting proof failures.
- The existing unrelated dirty README changes remain outside this slice.

## Proof

- Passed: `cmake --preset default && cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^(frontend_hir_tests|backend_lir_to_bir_interface|inline_asm_aarch64_simple)$'
  > test_after.log 2>&1`.
- Canonical proof log: `test_after.log` (3/3 tests passed).
