Status: Active
Source Idea Path: ideas/open/152_bir_producer_source_identity_foundation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch At Most One Narrow Consumer

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: switched exactly one narrow read-only identity
consumer, `find_prepared_same_block_select_producer`, to use the BIR
same-block producer identity query for already-proven select producer facts.

The switch is confined to the internal select producer lookup in
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`. It now asks
`mir::find_same_block_producer_identity` for the current BIR block, value name,
value type, and before-instruction boundary, then accepts only a materializable
`SameBlockProducerKind::Select` record that still points at the matching
`bir::SelectInst`.

Prepared scalar producer and constant queries remain available elsewhere as the
oracle/fallback surface. No broad AArch64 emission path, prepared publication
lookup owner, source idea, plan file, or test expectation was rewritten.

Added targeted AArch64 dispatch coverage showing the public helper finds a
same-block select through BIR identity without any prepared source-producer
fact, while still rejecting before-boundary and type-mismatch cases.

## Suggested Next

Execute Step 6 as the next bounded packet: assemble the close-ready evidence
summary in `todo.md`, including the BIR vocabulary/query API, equivalence proof
coverage, the single consumer switch, prepared-oracle availability, rejected
target-specific state, and final matched proof logs.

## Watchouts

- This packet intentionally switched only the select producer identity helper.
  Do not treat it as permission to move scalar producer recursion, constant
  evaluation, storage, register, or publication ownership in the same route.
- Prepared publication source-producer facts are still used by other helpers,
  including scalar producer and select-chain dependency routes.
- The helper name remains `find_prepared_same_block_select_producer` for API
  compatibility even though its select identity read now comes from BIR.
- Leave `review/phase_a_steps_1_4_route_review.md` untouched.

## Proof

Delegated proof command ran successfully and refreshed `test_after.log`:

```bash
set -o pipefail && cmake --build build --target backend_x86_shared_producer_query_test backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_x86_shared_producer_query|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: 3/3 passed:
`backend_x86_shared_producer_query`,
`backend_prepared_lookup_helper`, and
`backend_aarch64_instruction_dispatch`.
