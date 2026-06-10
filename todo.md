Status: Active
Source Idea Path: ideas/open/159_bir_producer_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Run acceptance validation and route review check

# Current Packet

## Just Finished

Step 4 - Prove Oracle Equivalence completed.

Extended `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` with
explicit Route 1 materialization-availability oracle checks against
`find_prepared_same_block_scalar_producer`.

The same-block scalar fixture now demonstrates BIR Route 1 answers match the
prepared oracle for:

- same-block scalar producer identity across binary, cast, load-local,
  load-global, and select-materialization producer kinds.
- integer constants for immediates and same-block binary constants.
- materialization availability across all supported scalar producer kinds.
- fail-closed future producer, missing fact, and mismatched value-type shapes.

Step 3 already added the scalar producer, integer constant, and negative
fail-closed comparisons in this fixture; Step 4 kept those checks and added the
missing all-kind materialization-availability oracle equivalence. No production
consumers were switched.

## Suggested Next

Execute Step 5 as the next packet:

- Switch at most one low-risk identity consumer to the BIR-backed Route 1 query.
- Keep the prepared query available as an oracle or fallback during migration.
- Avoid AArch64 emission rewrites and any consumer that needs storage, register,
  frame, publication hook, or final instruction facts.

## Watchouts

- Step 4 intentionally did not replace existing MIR/prepared query call sites
  or AArch64 consumers.
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
