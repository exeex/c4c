Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define BIR Memory Access Identity Records

# Current Packet

## Just Finished

Step 2 direct BIR memory-access identity surface is complete.

Implemented `BirMemoryAccessIdentityRequest`,
`BirMemoryAccessIdentity`, `BirMemoryAccessNodeKind`,
`BirMemoryAccessBaseKind`, `bir_memory_access_node_kind`, and
`find_bir_memory_access_identity` in `src/backend/mir/query.hpp/.cpp`.
The query is keyed by BIR block label, instruction index, and direct memory
node kind for `LoadLocalInst`, `LoadGlobalInst`, `StoreLocalInst`, and
`StoreGlobalInst`.

The record carries only semantic BIR identity:
result/stored value name, address space, volatile flag, semantic base kind,
BIR local slot name/id, global link/text spelling/id, pointer value name, and
string constant spelling. It intentionally does not carry prepared frame slot
ids, object ids, byte offsets, size/align, target addressing legality,
relocation/GOT/TLS policy, register homes, or storage encoding.

Added test-only prepared/BIR comparisons:
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  now checks direct load/store BIR memory identities against prepared semantic
  oracle fields for local, global, pointer, string, volatile, missing prepared
  access, result mismatch, and stored-value mismatch cases.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` now verifies the
  BIR direct memory identity lookup key behavior for matching load-local and
  store-global nodes plus node-kind, block-label, and missing-index rejection.

No consumers were switched to the BIR query in this packet.

## Suggested Next

Next coherent packet: add a BIR-owned same-block direct global-load identity
query on top of the direct memory identity surface, compare it against the
prepared same-block global-load oracle, and keep consumers on prepared queries
until equivalence is proven.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared memory/access queries available as the oracle until broader
  BIR equivalence is proven.
- Do not switch `dispatch_value_materialization.cpp`,
  `fp_value_materialization.cpp`, `alu.cpp`, `calls.cpp`, `globals.cpp`, or
  memory-retargeting consumers before the supervisor delegates that consumer
  migration.
- The current BIR direct memory identity intentionally treats missing
  structured `MemoryAddress` as a present direct memory node with default/no
  address identity fields; prepared/BIR comparison helpers reject missing
  prepared oracle entries separately.
- `find_prepared_same_block_load_local_stored_value_source` and
  `find_prepared_same_block_load_local_source_producer` currently depend on
  stack-layout range matching/overlap for correctness. A BIR query can expose
  the semantic source answer after equivalence, but should not own frame
  offsets, concrete sizes, or overlap math as BIR data.

## Proof

`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' > test_after.log 2>&1`

Passed. `test_after.log` contains the delegated proof log with both selected
tests passing.
