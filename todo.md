Status: Active
Source Idea Path: ideas/open/184_phase_e_route1_producer_constant_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate the selected consumer route-first

# Current Packet

## Just Finished

Step 2 exposed a narrow Route 1 publication-source view for the selected
`value_publication_may_read_register_index(...)` path in
`src/backend/mir/aarch64/codegen/dispatch_producers.hpp` and
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

The new `route1_publication_source_producer_for_value(...)` boundary is keyed
by `*context.bir_block`, `before_instruction_index`, and the consuming
`bir::Value`. It builds a Route 1 producer index for the current BIR block,
queries `bir::Route1SameBlockProducerQuery`, and returns stable semantic facts:
producer instruction, produced value, producer kind, instruction index,
materialization availability, and integer-constant status/value.

Absence and fallback cases remain explicit:

- `Unavailable` covers no current BIR block or non-named/empty values.
- `NoProducer` covers valid same-block Route 1 queries with no before-consumer
  producer, including future/cross-block/mismatched-type cases.
- `NonConstant` and `Constant` distinguish producer records without copying any
  prepared homes, registers, storage, moves, or machine-record policy.
- Existing prepared helper APIs and the
  `prepared_same_block_publication_source_producer(...)` fallback/oracle helper
  remain available and behaviorally unchanged.

## Suggested Next

Migrate `value_publication_may_read_register_index(...)` to use
`route1_publication_source_producer_for_value(...)` first, with the existing
prepared helper retained as fallback/oracle for route-unavailable or
intentionally out-of-scope cases.

## Watchouts

The Step 2 view intentionally does not change the recursive publication
dependency walk yet. Step 3 should avoid treating `NoProducer` as a hard
prepared bypass unless the route is authoritative for the selected case; keep
prepared fallback behavior for route-unavailable/out-of-scope answers.

## Proof

Step 2 ran and passed:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$'; } > test_after.log 2>&1`

Accepted proof log: `test_before.log` after supervisor roll-forward.
