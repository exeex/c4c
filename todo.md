# Current Packet

Status: Active
Source Idea Path: ideas/open/779_lir_cast_result_authority_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add focused positive and malformed cast coverage

## Just Finished

- Plan Step 1 complete: added an explicit `LirCastOp` standalone native-result
  ownership contract. The verifier rejects a selected cast without
  `LirValueId` result authority; present IDs continue through the existing
  function ownership checks for invalid and duplicate IDs and value-use scope.

## Suggested Next

- Dispatch Plan Step 2: add a standalone cast fixture that selects
  `requires_native_result_authority` and covers the positive, missing,
  invalid, duplicate, and foreign-ID rejection paths.

## Watchouts

- The explicit flag is required because source-ID presence alone also occurs
  in the excluded logical RHS cast path, whose result authority remains raw.
- Keep PHI, logical producer lowering, generic expression APIs, and the
  unaccepted 778 `binary.cpp` diff out of the next packet.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
  Focused test output: `test_after.log`.
