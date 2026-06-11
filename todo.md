Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Route and Return-Chain Shapes

# Current Packet

## Just Finished

Step 1 inspection completed. The new BIR-owned return-chain schema/index should
be added alongside existing BIR route records in `src/backend/bir/bir.hpp` and
implemented in `src/backend/bir/bir.cpp`; `src/backend/mir/query.hpp` and
`src/backend/mir/query.cpp` are the MIR-facing bridge targets only if the next
packet chooses to expose a public query wrapper.

Concrete BIR insertion targets:

- Add a distinct Route 8 family after Route 7 in `src/backend/bir/bir.hpp`:
  `Route8ReturnChainStatus`, `Route8ReturnChainValueKey`,
  `Route8ReturnChainRecord`, `Route8ReturnChainIndex`, and lookup/query helpers.
- Implement the helpers in `src/backend/bir/bir.cpp`, following the vector
  record/index style of `route6_build_call_use_source_index(...)` and
  `route7_build_comparison_condition_index(...)`, not the prepared-only
  unordered-map storage.
- Expected helper names: `route8_return_chain_value_key(...)`,
  `route8_return_chain_record(...)`, `route8_build_return_chain_index(...)`,
  `route8_find_return_chain_record(...)`,
  `route8_find_return_chain_terminal_value(...)`, and
  `route8_find_return_chain_next_operand_value(...)`.
- The key should preserve function/block ownership, block label/id,
  instruction index, and the chain `Route1SourceValueIdentity`/named value id.
  It should answer terminal return value identity and optional immediate
  next-operand identity without target homes, registers, ABI return placement,
  scratch policy, or consumer emission facts.

Prepared helper shape inspected:

- `PreparedReturnChainLookups` lives in
  `src/backend/prealloc/prepared_lookups.hpp` as two maps:
  `terminal_return_values_by_chain_value` and
  `next_operand_values_by_chain_value`.
- The prepared key is `prepared_return_chain_value_key(block_index,
  instruction_index, ValueNameId)`.
- Construction is in `make_prepared_return_chain_lookups(...)`; it walks
  same-block scalar `BinaryInst` chains in return-terminated blocks, publishes
  through `publish_prepared_return_chain(...)`, and marks conflicting duplicate
  terminal/next answers as `kInvalidValueName` instead of choosing a winner.
- Lookup APIs to preserve as oracle targets are
  `find_prepared_return_chain_terminal_value(...)` and
  `find_prepared_return_chain_next_operand_value(...)`.

Focused test target:

- Host new fixtures in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  as `verify_bir_return_chain_identity_lookup()` and call it from `main()`.
- Existing CMake target/test remain `backend_prepared_lookup_helper_test` and
  `backend_prepared_lookup_helper`.
- Future implementation proof subset should be:
  `cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure`.

## Suggested Next

Execute Step 2 by adding the Route 8 return-chain status/key/record/index type
declarations in `src/backend/bir/bir.hpp` and the minimal skeleton helpers in
`src/backend/bir/bir.cpp`, without changing prepared helpers or consumers.

## Watchouts

- Keep the route target-neutral: no homes, registers, ABI placement, scratch
  policy, alias checks, ALU record construction, or emission order.
- Do not migrate AArch64 consumers or hide prepared return-chain helpers in
  this idea.
- Preserve the prepared conflict behavior semantically: conflicting duplicate
  return-chain answers must fail closed rather than picking one publication.
- Route 8 should stay distinct from Route 1 producer identity and Route 7
  comparison provenance; reuse `Route1SourceValueIdentity` for values, not the
  Route 1 index as the route itself.
- Keep oracle equivalence against prepared helpers for idea 178.
- No blocker found. Naming decision recorded: use `Route8ReturnChain...` for
  the new BIR route family.

## Proof

Metadata/inspection-only packet; no build or CTest run required by the
delegated proof. No `test_after.log` update was made for this packet.
