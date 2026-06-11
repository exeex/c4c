Status: Active
Source Idea Path: ideas/open/180_bir_return_chain_prepared_api_contraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retarget Tests And Diagnostics

# Current Packet

## Just Finished

Plan Step 4 - Retarget Tests And Diagnostics is complete as a
verification-only packet. Focused inspection found that Step 3 already
retargeted the Route 8 return-chain coverage and diagnostics; no code, test,
or diagnostic edits are needed for Step 4.

Verification performed:

- Inspected `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  around `verify_bir_return_chain_schema_and_index_lookup`. The coverage now
  uses `bir::route8_build_return_chain_index`,
  `bir::route8_return_chain_value_key`,
  `bir::route8_find_return_chain_record`,
  `bir::route8_find_return_chain_terminal_value`, and
  `bir::route8_find_return_chain_next_operand_value`.
- Confirmed the Route 8 test covers positive terminal/next-operand identity,
  block-local indexing, unsupported opcode, unnamed chain value, unnamed
  terminal value, broken same-block walk, non-return terminator, cross-block
  boundary, missing instruction keys, duplicate terminal conflict, duplicate
  next-operand conflict, and duplicate conflicting record fail-closed behavior.
- Search for removed prepared return-chain API names returned no matches in
  `src` or `tests`.
- Search for return-chain helper/authority wording found Route 8-only test
  diagnostics plus the AArch64 comment that Route 8 is the semantic
  return-chain source and prepared access is limited to value-home
  translation.

## Suggested Next

Proceed to Step 5 validation. Suggested packet: run the supervisor-selected
acceptance proof for the contraction, likely the prepared lookup helper test
plus the AArch64 return-lowering subset or broader regression guard if the
supervisor wants milestone confidence.

## Watchouts

- `rg -n "PreparedReturnChainLookups|return_chains\\b|prepared_return_chain_value_key|make_prepared_return_chain_lookups|find_prepared_return_chain_terminal_value|find_prepared_return_chain_next_operand_value" src tests`
  exited with status 1 and no matches.
- `rg -n "prepared return-chain|prepared return chain|return-chain helper|return-chain helpers|prepared return-chain helper|prepared return chain helper|prepared.*return-chain.*oracle|return-chain.*prepared.*oracle|prepared.*return-chain.*authority|return-chain.*prepared.*authority|Route 8.*prepared|prepared.*Route 8" src tests`
  found only Route 8 helper wording in
  `backend_prepared_lookup_helper_test.cpp` and the AArch64 comment in
  `src/backend/mir/aarch64/codegen/alu.cpp`.
- The remaining "Route 8 return-chain helper(s)" diagnostic text describes
  `bir::route8_*` helpers, not prepared/prealloc helper authority.

## Proof

No build or test proof was delegated for this verification-only packet.
`test_after.log` was not modified.

Inspection commands used:

```sh
sed -n '9720,10130p' tests/backend/bir/backend_prepared_lookup_helper_test.cpp
rg -n "PreparedReturnChainLookups|return_chains\\b|prepared_return_chain_value_key|make_prepared_return_chain_lookups|find_prepared_return_chain_terminal_value|find_prepared_return_chain_next_operand_value" src tests
rg -n "prepared return-chain|prepared return chain|return-chain helper|return-chain helpers|prepared return-chain helper|prepared return chain helper|prepared.*return-chain.*oracle|return-chain.*prepared.*oracle|prepared.*return-chain.*authority|return-chain.*prepared.*authority|Route 8.*prepared|prepared.*Route 8" src tests
rg -n "Route 8|return-chain|return chain" src/backend tests/backend/bir tests/backend/mir
```
