Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define the structured LIR inline-asm value contract

# Current Packet

## Just Finished

- Completed Plan Step 1: added the structured `LirInlineAsmOp` ordinary-value
  contract, explicit original semantic text fields, role/order/type/identity
  verification, and focused input/output/read-write model coverage.

## Suggested Next

- Execute Plan Step 2 by populating original semantic text and ordinary
  input/result bindings in HIR-to-LIR while retaining LLVM compatibility
  rendering for the current printer.

## Watchouts

- `LirInlineAsmValueBinding::value` is an ordinary `LirValueId`; keep producer
  and importer wiring on that identity rather than introducing another value
  family.
- HIR-to-LIR does not populate the new semantic fields yet, and LIR-to-BIR does
  not consume them yet. Until Steps 2 and 3 land, manually structured void LIR
  asm must not be treated as losslessly imported by the current adapter.
- The existing unrelated dirty README changes remain outside this slice.

## Proof

- Passed: `cmake --preset default && cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R
  '^(frontend_hir_tests|backend_lir_to_bir_interface)$' > test_after.log 2>&1`.
- Canonical proof log: `test_after.log` (2/2 tests passed).
