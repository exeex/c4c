# Current Packet

Status: Active
Source Idea Path: ideas/open/785_lir_call_result_operand_carrier_foundation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory direct call-result carrier surfaces and probe feasibility

## Just Finished

- Lifecycle switch from 784 Step 2 after its reverted, unaccepted GP/FP
  carrier attempt exposed `make_lir_call_op` as the first unresolvable native
  result loss in the FP128 ptrmask route. No code or test change is accepted.

## Suggested Next

- Plan Step 1: inventory the direct factory and support type boundaries; decide
  whether native `LirOperand` transport can remain outside unrelated generic
  call/argument families.

## Watchouts

- This is direct generic call-result transport only, not PHI, generic-expression,
  Raw-BIR/importer, backend, target lowering, MIR, emission, or vaarg helper work.
- If direct support necessarily reaches unrelated generic call/argument
  families, stop and name the exact narrower successor.

## Proof

- Preserved accepted focused proof: `cmake --build --preset default && ctest
  --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
  passed 1/1 after reverted partial work; full output is `test_after.log`.
  `test_before.log` and `test_after.log` match; no code or test change is
  accepted.
