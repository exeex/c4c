Status: Active
Source Idea Path: ideas/open/178_bir_return_chain_oracle_equivalence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Oracle Surfaces and Fixture Keys

# Current Packet

## Just Finished

Step 1 inspection completed. The shared oracle key should use the same
logical tuple on both routes:

- function: Route 8 uses `route8_build_return_chain_index(function)` plus
  `route8_return_chain_value_key(&function, block, instruction_index, value,
  value_name_id)`; prepared uses the matching
  `PreparedControlFlowFunction::function_name`/`PreparedBirModule::module`
  function selected by `make_prepared_return_chain_lookups(prepared,
  control_flow)`.
- block: Route 8 carries the concrete `bir::Block&` plus label/label id in
  `Route8ReturnChainValueKey`; prepared uses the same block position as
  `block_index` and the same interned block label in
  `PreparedControlFlowFunction::blocks`.
- instruction position: both routes use the same `instruction_index` inside
  the BIR block.
- chain value: Route 8 should be queried with the same `bir::Value` and its
  interned `ValueNameId` so `Route1SourceValueIdentity::name_id` can be
  compared directly to prepared helper answers; prepared queries use that same
  `ValueNameId`.

Existing fixture to extend:
`verify_bir_return_chain_schema_and_index_lookup()` in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`. It already owns
the positive Route 8 same-block chain (`%seed` at instruction 0, `%ret` at
instruction 1, `%named.next` as immediate next operand), the terminal no-next
case, rejected shapes, cross-block rejection, missing-instruction status, and
manual duplicate conflict coverage.

Helper APIs recorded for the next packet:

- Route 8: `route8_build_return_chain_index(const Function&)`,
  `route8_return_chain_value_key(...)`,
  `route8_find_return_chain_record(...)`,
  `route8_find_return_chain_terminal_value(...)`, and
  `route8_find_return_chain_next_operand_value(...)`.
- Prepared: `make_prepared_return_chain_lookups(prepared, control_flow)`,
  `find_prepared_return_chain_terminal_value(&lookups, block_index,
  instruction_index, value_name_id)`, and
  `find_prepared_return_chain_next_operand_value(&lookups, block_index,
  instruction_index, value_name_id)`.

Prepared fixture strategy: inside the existing test, create a
`PreparedBirModule`, intern the function name, block label, `%seed`,
`%named.next`, and `%ret`, push the same BIR `function` into
`prepared.module.functions`, create a one-block `PreparedControlFlowFunction`
using `return_block(entry_label)`, then compare prepared `ValueNameId` answers
with Route 8 identities queried using keys built with those same value ids.
No blockers found before code changes.

## Suggested Next

Execute Step 2 by extending
`verify_bir_return_chain_schema_and_index_lookup()` with the shared prepared
fixture and positive terminal equivalence assertions for `%seed` and `%ret`.

## Watchouts

- Keep prepared return-chain helpers public and unchanged during this idea.
- Do not migrate AArch64 consumers or contract the prepared API.
- Compare Route 8 against prepared helper answers for terminal and
  next-operand queries.
- Rejected, ambiguous, and duplicate-conflict fixtures must fail closed rather
  than choosing a winner.
- Prepared helpers return `kInvalidValueName` for no answer or conflicting
  duplicate answers; Route 8 helper identities return an empty
  `Route1SourceValueIdentity` when the record is not available.
- Query Route 8 with interned `ValueNameId`s in oracle-equivalence assertions;
  otherwise only spellings match and the prepared id comparison is weaker.

## Proof

Inspection-only metadata packet; no build required and no `test_after.log`
created by this delegated proof contract.
