Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Prepared/BIR Query Equivalence

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: expanded explicit prepared/BIR query-equivalence
proof in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` without
switching any consumers.

Equivalence evidence now covers:
- same-block scalar producer answers for binary `%lhs`, `%rhs`, `%sum`, and
  `%product` producers;
- same-block scalar producer answers for cast `%wide`, load-local `%from_slot`,
  load-global `%from_global`, and select `%choice` producers;
- same-block integer constant answers for immediate constants and binary
  constant producers `%lhs`, `%rhs`, `%sum`, and `%product`;
- fail-closed integer-constant agreement for nonconstant scalar producers
  `%wide`, `%from_slot`, `%from_global`, and `%choice`;
- fail-closed scalar/constant behavior for future `%product`, missing values,
  BIR mismatched value type, and prepared mismatched source-producer facts.

Prepared queries remain the oracle, and no AArch64 consumer implementation or
other consumer path was switched.

## Suggested Next

Execute Step 5 as the next bounded packet only if the supervisor accepts the
equivalence proof: select at most one narrow read-only identity consumer and
switch only the already-proven identity facts to the BIR query, with prepared
queries still available as oracle/fallback during this idea.

## Watchouts

- Prepared/BIR equivalence is now a targeted unit proof, not a broad backend
  migration. Consumer switching should remain narrow and read-only.
- Integer-constant equivalence intentionally proves the binary/immediate subset
  shared by both query surfaces; cast, load, and select producers fail closed
  for constant evaluation while still proving scalar producer identity.
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
