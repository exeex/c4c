Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Target-Neutral BIR Producer Vocabulary

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: defined the first target-neutral BIR producer
identity vocabulary and record shape around the existing MIR/BIR same-block
query surface without switching consumers.

Implemented in `src/backend/mir/query.hpp` and `src/backend/mir/query.cpp`:
- `SameBlockProducerIdentityRequest` captures the BIR query tuple:
  block, block label spelling, value name, optional value type, and
  before-instruction index.
- `SameBlockValueIdentity` captures value pointer, name, type, and immediate
  integer constant availability as BIR value facts.
- `SameBlockProducerRecord` now also carries block label,
  before-instruction index, produced value identity, and semantic producer
  materialization availability.
- `SameBlockProducerIdentity` aliases the richer record shape for the new
  query vocabulary.
- `same_block_producer_kind_has_materialization(...)` classifies only BIR
  producer kinds, not prepared publication/register/storage state.
- `same_block_value_identity(...)` exposes named/immediate value facts without
  treating immediates as producer instructions.
- `find_same_block_producer_identity(...)` answers same-block BIR producer
  identity from BIR instructions and fails closed for missing names, future
  producers, and mismatched requested types.
- Existing `find_same_block_named_producer_record(...)` now routes through the
  richer identity query, preserving old callers while making the new record
  shape reachable.

Focused coverage added in
`tests/backend/mir/backend_x86_shared_producer_query_test.cpp`:
- binary producer and recursive integer constant baseline still pass.
- immediate constants are represented as value facts.
- cast records expose name/type/block/before-index/producer pointer/index and
  materialization availability.
- load-local and load-global producer identities classify through the new
  BIR vocabulary.
- future producer, missing name, and mismatched type cases fail closed.
- select-chain dependency traversal remains available.

Prepared/prealloc/publication/register/storage/final-emission authority was not
imported into the new MIR/BIR query vocabulary, and no consumer implementation
was switched.

## Suggested Next

Execute Step 3 as the next code-changing packet: add BIR-owned same-block query
APIs that compare directly against the prepared scalar producer and integer
constant oracle, still without switching AArch64 consumers.

## Watchouts

- `evaluate_same_block_integer_constant(...)` still lacks an explicit
  `before_instruction_index`; Step 3/4 should add or wrap before-index-aware
  constant query behavior before claiming full prepared equivalence.
- The new identity query intentionally preserves the prepared oracle rather
  than replacing it; consumer switching remains out of scope until equivalence
  is proven.
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
