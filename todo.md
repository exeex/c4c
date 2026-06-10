Status: Active
Source Idea Path: ideas/open/159_bir_producer_identity_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Run acceptance validation and route review check

# Current Packet

## Just Finished

Step 5 - Switch One Low-Risk Identity Consumer completed.

Switched the generic MIR same-block producer identity layer to read from the
BIR Route 1 record/query surface:

- `find_same_block_producer_identity` now builds a transient
  `bir::Route1ProducerIndex`, scans Route 1 records, and translates the matched
  BIR producer record back into the existing MIR identity result.
- `find_same_block_scalar_producer` now delegates to
  `bir::route1_find_same_block_scalar_producer`.
- `evaluate_same_block_integer_constant` now delegates to
  `bir::route1_evaluate_same_block_integer_constant`; BIR exposes a small
  depth-taking adapter overload so MIR does not keep an independent recursive
  constant evaluator.

No AArch64 files, emission paths, prepared production helpers, or expectation
contracts were changed.

## Suggested Next

Execute Step 6 as the next packet:

- Run the supervisor-selected acceptance validation and route review check.
- Confirm the Step 5 adapter switch remains semantically aligned with the
  source idea and does not create route drift through shared AArch64 callers.
- If accepted, ask the plan owner to decide whether to close this source idea
  or split remaining work into follow-up schema/consumer-switch ideas.

## Watchouts

- Step 5 changed shared MIR query behavior, so Step 6 should treat the slice as
  code-changing and validate beyond docs-only checks.
- `Route1ProducerIndex` is still rebuilt per query and stores record copies
  with pointers into the source `bir::Block`; rebuild the index after mutating
  a block.
- Immediate constants remain value facts, not standalone instruction producer
  records.
- Keep materialization payload semantic only. Do not add prepared homes,
  registers, storage, frame slots, emitted availability, spill/reload behavior,
  operand views, or final instruction records.
- `clang-format` is not installed in this environment; manual style hygiene was
  used for this packet.

## Proof

Ran exact delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_x86_shared_producer_query)$'`

Result: passed. Full proof output saved in `test_after.log`.

`git diff --check` also passed.
