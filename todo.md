Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add BIR-Owned Same-Block Producer Queries

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: added BIR-owned same-block query APIs that can
be compared directly against prepared same-block scalar producer and integer
constant answers without switching consumers.

Implemented in `src/backend/mir/query.hpp` and `src/backend/mir/query.cpp`:
- `SameBlockValueMaterializationQuery` captures the BIR query tuple:
  block, block label spelling, and before-instruction index.
- `SameBlockScalarProducer` exposes the BIR-owned scalar producer identity,
  instruction pointer, produced value pointer, and producer instruction index.
- `find_same_block_scalar_producer(...)` answers value-based same-block scalar
  producer queries through the existing BIR identity record and fails closed
  for missing names, mismatched types, or producers after the requested index.
- `evaluate_same_block_integer_constant(SameBlockValueMaterializationQuery, ...)`
  evaluates integer constants with the same before-index recursion rule used by
  the prepared oracle.
- The existing block/value integer constant entry point remains available and
  now delegates through the before-index-aware query over the full block.

Focused coverage added in
`tests/backend/mir/backend_x86_shared_producer_query_test.cpp`:
- before-index BIR scalar producer and integer constant queries reject future
  producers.
- before-index BIR integer constant queries fold eligible producers inside the
  requested window.

Prepared-oracle comparison coverage added in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
- BIR same-block scalar producer results match prepared scalar producer
  instruction pointer and instruction index.
- BIR same-block integer constant results match prepared integer constant
  answers.
- BIR and prepared queries both fail closed for a future `%product` producer.

Prepared queries remain available as the oracle, and no AArch64 consumer
implementation was switched.

## Suggested Next

Execute Step 4 as the next code-changing packet: add a target-neutral
equivalence harness or comparison helper that can keep proving BIR-owned
same-block query answers against prepared query answers before any consumer
switch.

## Watchouts

- The BIR-owned query path intentionally preserves the prepared oracle rather
  than replacing it; consumer switching remains out of scope until equivalence
  proof is broader than the focused Step 3 fixture.
- Keep immediates as `SameBlockValueIdentity::immediate_constant` facts, not
  producer records.
- Leave `review/phase_a_steps_1_4_route_review.md` untouched.

## Proof

Delegated proof command ran successfully and wrote `test_after.log`:

```bash
set -o pipefail && cmake --build build --target backend_x86_shared_producer_query_test backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_x86_shared_producer_query|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: 3/3 passed:
`backend_x86_shared_producer_query`,
`backend_prepared_lookup_helper`, and
`backend_aarch64_instruction_dispatch`.
