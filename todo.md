Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Review And Close Payload

# Current Packet

## Just Finished

Completed Step 6 of `plan.md`: assembled the close-ready acceptance evidence
for the BIR producer/source identity foundation.

The implemented target-neutral vocabulary is in `src/backend/mir/query.hpp`:
`SameBlockProducerKind`, `SameBlockProducerIdentityRequest`,
`SameBlockValueIdentity`, `SameBlockProducerRecord` /
`SameBlockProducerIdentity`, `SameBlockValueMaterializationQuery`,
`SameBlockScalarProducer`, and `SameBlockIntegerConstant`. The accepted
producer kinds are semantic BIR instruction kinds only: `Binary`, `Cast`,
`Select`, `LoadLocal`, and `LoadGlobal`; `Unknown` remains non-materializable.

The new query API is BIR-owned and target-neutral:
`find_same_block_producer_identity(...)` records block label,
before-instruction index, value name/type, producer instruction/index,
produced-value identity, immediate constants only as value facts, and
materialization availability. `find_same_block_scalar_producer(...)` and
`evaluate_same_block_integer_constant(...)` build on that identity surface while
respecting same-block before-index boundaries.

Equivalence proof covers prepared/BIR agreement for same-block scalar producers
across binary, cast, load-local, load-global, select, and product cases. Integer
constant equivalence covers immediate constants and binary constant folding.
Fail-closed coverage includes future producers, missing names/facts, mismatched
value types, mismatched prepared facts, and nonconstant scalar producers.

Exactly one narrow consumer was switched:
`find_prepared_same_block_select_producer` now reads select producer identity
through `mir::find_same_block_producer_identity`. The switch accepts only a
materializable `SameBlockProducerKind::Select` record with matching BIR
instruction, instruction boundary, value name, and value type. Prepared scalar
producer and integer-constant query surfaces remain available as the oracle and
fallback surface for later routes.

Rejected target-specific state stayed out of the BIR relationship. The new BIR
identity records do not carry register homes, storage state, publication-owned
enum authority, emitted-register availability, spill/reload behavior, operand
views, frame-slot layout, or final instruction order.

## Suggested Next

Supervisor lifecycle closure review for
`ideas/open/152_bir_producer_source_identity_foundation.md`.

## Watchouts

- This idea intentionally switched only the select producer identity helper.
  Later routes still need separate lifecycle packets for scalar producer
  recursion, constant evaluation, storage, register, publication, or emission
  ownership changes.
- Prepared publication source-producer facts are still used by other helpers,
  including scalar producer and select-chain dependency routes.
- The helper name remains `find_prepared_same_block_select_producer` for API
  compatibility even though its select identity read now comes from BIR.
- Leave `review/phase_a_steps_1_4_route_review.md` untouched.

## Proof

Docs/lifecycle-only Step 6 packet; no build was required or run for this
closure-evidence update.

Final matched code-scope proof command recorded for closure:

```bash
set -o pipefail && cmake --build build --target backend_x86_shared_producer_query_test backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_x86_shared_producer_query|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Canonical matched logs exist:

- `test_before.log`: 3/3 passed:
  `backend_x86_shared_producer_query`,
  `backend_prepared_lookup_helper`, and
  `backend_aarch64_instruction_dispatch`.
- `test_after.log`: 3/3 passed:
  `backend_x86_shared_producer_query`,
  `backend_prepared_lookup_helper`, and
  `backend_aarch64_instruction_dispatch`.

Supervisor-reported regression guard status:
`c4c-regression-guard --allow-non-decreasing-passed` passed for the matched
logs.

Final proof subset:
`backend_x86_shared_producer_query`,
`backend_prepared_lookup_helper`, and
`backend_aarch64_instruction_dispatch`.
