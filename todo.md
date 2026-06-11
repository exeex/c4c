Status: Active
Source Idea Path: ideas/open/177_bir_return_chain_schema_index.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Return-Chain Record and Key Types

# Current Packet

## Just Finished

Step 2 completed. Added compileable BIR Route 8 return-chain declarations in
`src/backend/bir/bir.hpp`: `Route8ReturnChainStatus`,
`Route8ReturnChainValueKey`, `Route8ReturnChainRecord`,
`Route8ReturnChainIndex`, status naming, key construction, record construction,
index build overloads, record find, terminal value find, and next-operand value
find declarations.

Implemented minimal skeleton helpers in `src/backend/bir/bir.cpp`. The Route 8
key preserves function pointer/name/link id when present, block pointer/label/id,
instruction index, and chain `Route1SourceValueIdentity`. Records carry terminal
return identity plus optional next-operand identity only. The builder overloads
return owner-scoped empty indexes and do not traverse return chains yet. Find
helpers fail closed with status-coded records and return empty identities until
future traversal publishes records.

## Suggested Next

Execute the next Step 3 packet by adding focused BIR tests for the Route 8
schema/key/record skeleton and oracle-facing lookup shape, without changing
prepared helpers, AArch64 consumers, or implementing full return-chain traversal
unless the supervisor delegates that expansion.

## Watchouts

- Keep the route target-neutral: no homes, registers, ABI placement, scratch
  policy, alias checks, ALU record construction, or emission order.
- Do not migrate AArch64 consumers or hide prepared return-chain helpers in this
  idea.
- Preserve the prepared conflict behavior semantically: conflicting duplicate
  return-chain answers must fail closed rather than picking one publication.
- Route 8 remains distinct from Route 1 producer identity and Route 7 comparison
  provenance; it reuses `Route1SourceValueIdentity` for values, not the Route 1
  index as the route itself.
- Keep oracle equivalence against prepared helpers for idea 178.
- Current Route 8 indexes are intentionally empty skeletons; traversal and
  publication are not implemented yet.

## Proof

Ran the delegated proof command:
`(cmake --build build --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1`

Result: passed. The `backend_prepared_lookup_helper_test` target built and CTest
reported `backend_prepared_lookup_helper` passed. Proof log: `test_after.log`.
