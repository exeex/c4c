Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Same-Block Value-Name Contract

# Current Packet

## Just Finished

Completed Step 1: inventory for idea 260's `names` same-block value-name
lookup candidate.

Definitions/declarations found:

- `src/backend/prealloc/prepared_lookups.cpp` has the private
  `prepared_same_block_source_producer(...)` helper plus public
  `find_prepared_same_block_scalar_producer(...)` and
  `evaluate_prepared_same_block_integer_constant(...)` overloads declared in
  `prepared_lookups.hpp`. The path resolves a named BIR value through
  `PreparedNameTables::value_names`, indexes
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name`,
  requires same prepared block label, producer instruction before cutoff, an
  in-block instruction pointer match, result pointer match, result type match,
  and prepared value-name match.
- `src/backend/prealloc/select_chain_lookups.cpp` has the local
  `find_prepared_select_chain_source_producer(...)` implementation declared in
  `select_chain_lookups.hpp`. It uses the same source-producer lookup map but
  resolves by value spelling, then requires same block label, before-cutoff
  producer index, in-block instruction pointer match, named result spelling,
  and result type match.
- Direct BIR/MIR query equivalents live in `src/backend/mir/query.hpp` and
  `src/backend/mir/query.cpp`: `find_same_block_named_producer_record(...)`,
  `find_same_block_producer_identity(...)`,
  `find_same_block_scalar_producer(...)`, and
  `evaluate_same_block_integer_constant(...)`. These delegate to Route 1
  producer records and fold immediates/binary integer constants.
- `src/backend/prealloc/publication_plans.hpp` declares the shared
  `PreparedEdgePublicationSourceProducer`,
  `PreparedEdgePublicationSourceProducerLookups`, and
  `find_prepared_same_block_load_local_source_producer(...)` contract used by
  the same-block/select-chain consumers.

Direct callers and consumers:

- Prepared comparison paths call the selected scalar/integer helpers in
  `src/backend/prealloc/comparison.cpp` for fused compare operands and
  materialized condition producers.
- Call argument materialization calls the scalar helper in
  `src/backend/prealloc/call_plans.cpp`.
- Store-source and direct-global select-chain planning call the select-chain
  helper in `src/backend/prealloc/publication_plans.cpp`.
- MIR/AArch64 dispatch paths also consume the scalar/integer query family in
  `src/backend/mir/aarch64/codegen/*`; they are broader consumers, not the
  selected Step 2 design surface.

Current handling:

- Unnamed values and empty names already fail closed before prepared value-name
  lookup in both prepared lookup files and in the direct BIR query facade.
- Missing prepared value ids fail closed for the selected prepared scalar and
  integer-constant lookups; select-chain currently resolves through prepared
  spelling as compatibility behavior.
- Stale producers and future producers fail closed through the producer index
  cutoff, in-block instruction bounds, and pointer/instruction-kind checks.
- Wrong result types fail closed for scalar and integer-constant lookups via
  result type agreement; existing tests cover mismatched `%sum` type queries.
- Mismatched prepared producer facts fail closed in the prepared path, while
  direct BIR same-block facts can still be block-derived; Step 2 should define
  when that block-derived answer is allowed to count as prepared/BIR agreement.
- Duplicate spelling and prepared/BIR name drift are the main unproven
  candidate-specific risk rows. Existing `publish_source_producer(...)` marks
  duplicate prepared value-name source-producer entries as `Unknown`, but the
  selected public same-block rows still need explicit coverage for duplicate
  BIR spelling and prepared-name drift.

Candidate implementation files:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/select_chain_lookups.cpp`
- possibly `src/backend/prealloc/lookup_agreement.hpp`
- possibly `src/backend/prealloc/lookup_agreement.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Suggested Next

Execute Step 2: design a narrow value-name agreement boundary shared by the
prepared same-block scalar/integer lookup path and the select-chain source
producer path. The design should accept only non-empty named BIR values whose
prepared `ValueNameId`, BIR spelling, type, same-block producer, producer
instruction index, and before-instruction cutoff agree; it should reject
unnamed/empty values, missing prepared ids, stale producers, wrong types,
duplicate spelling, and prepared/BIR name drift.

## Watchouts

- Keep this runbook limited to the selected `names` same-block value-name
  lookup candidate.
- Do not treat this activation as approval for `PreparedBirModule` deletion,
  privatization, wrapping, broad aggregate retirement, or another idea 260
  candidate.
- Preserve prepared name tables, existing lookup maps, public aggregate
  compatibility, and current null or unavailable fallback behavior.
- Preserve raw-spelling lookup only as compatibility behavior; do not treat it
  as structured prepared/BIR value-name agreement.
- Step 3/4 proof should add nearby rows in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for positive
  agreed scalar/integer lookups plus unnamed, empty, missing prepared id, stale
  producer, wrong type, duplicate spelling, and prepared/BIR name-drift
  rejection.
- Do not rewrite route-debug, target output, baselines, unsupported
  expectations, helper/oracle status names, printer/debug strings, value-home
  lookup, semantic resolver API, control-flow, or store-source publication
  behavior to claim progress.

## Proof

Recommended focused proof for Step 2/3:

```bash
(cmake --build --preset default --target backend_prepared_lookup_helper_test && ctest --test-dir build -R '^backend_prepared_lookup_helper$' --output-on-failure) > test_after.log 2>&1
```

Step 1 inventory proof:

```bash
git diff --check -- todo.md
```
