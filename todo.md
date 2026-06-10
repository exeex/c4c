Status: Active
Source Idea Path: ideas/open/159_bir_producer_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add BIR Same-Block Producer Queries And Indexes

# Current Packet

## Just Finished

Step 3 - Add BIR Same-Block Producer Queries And Indexes completed.

Added BIR-owned Route 1 same-block query/index surfaces in
`src/backend/bir/bir.hpp` / `src/backend/bir/bir.cpp`:

- `Route1ProducerIndex` as a cheap function-local index over typed
  `Route1ProducerRecord` payloads.
- `Route1SameBlockProducerQuery` and `Route1SameBlockScalarProducer` for
  same-block scalar producer lookup by named BIR value before a given
  instruction index.
- `route1_find_materialization_availability` over the Route 1 record payload.
- `route1_evaluate_same_block_integer_constant` for immediates and recursive
  same-block binary constants, using Route 1 producer records rather than
  prepared producer-kind authority.

Extended `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` so the
existing same-block scalar fixture compares Route 1 BIR-backed answers with
prepared oracle answers for supported producer kinds, materialization
availability, immediate/binary integer constants, future producers, missing
facts, and mismatched value types. No production consumers were switched.

## Suggested Next

Execute Step 4 as the next packet:

- Broaden oracle-equivalence coverage around the Route 1 APIs without switching
  production consumers.
- Add representative coverage for remaining supported producer/fail-closed
  shapes if the supervisor wants Step 4 separated from the Step 3 fixture
  extension.
- Keep prepared helpers as oracle checks only.

## Watchouts

- Step 3 added a BIR-owned Route 1 index API but intentionally did not replace
  existing MIR/prepared query call sites or AArch64 consumers.
- `Route1ProducerIndex` stores record copies with pointers into the source
  `bir::Block`; rebuild the index after mutating a block.
- Immediate constants remain value facts, not standalone instruction producer
  records.
- Keep materialization payload semantic only. Do not add prepared homes,
  registers, storage, frame slots, emitted availability, spill/reload behavior,
  operand views, or final instruction records.
- `clang-format` is not installed in this environment; manual style hygiene was
  used for this packet.

## Proof

Ran exact delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. Full proof output saved in `test_after.log`.
