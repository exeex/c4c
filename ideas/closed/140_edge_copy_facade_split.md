# 140 Edge-copy facade split

## Goal

Separate reusable edge publication/source/home/move facts from current-block
join parallel-copy routing conveniences that encode AArch64-shaped emission
details.

## Why This Exists

The Step 3 classification in the BIR/prealloc prepared query surface audit
found a mixed edge-copy surface. Indexed edge publication and move-bundle facts
are good shared semantics, but some helpers combine those facts with
lowering-routing conveniences:

- `prepare_current_block_join_parallel_copy_source_facts`
- `prepare_current_block_join_parallel_copy_instruction_routing`
- `prepare_same_width_i32_stack_source_publication`
- `prepare_aggregate_stack_source_authority`
- fields on `PreparedCurrentBlockJoinParallelCopySourceFact` and
  `PreparedCurrentBlockJoinParallelCopyInstructionRouting` that expose
  destination register names, same-source/destination-register routing, typed
  stack-source handling, or concrete publication route choices

Future x86/riscv backends should inherit edge publication facts without
inheriting AArch64 dispatch routing assumptions.

## In Scope

- Edge-copy query declarations and definitions in:
  - `src/backend/prealloc/prepared_lookups.hpp`
  - `src/backend/prealloc/prepared_lookups.cpp`
  - `src/backend/prealloc/control_flow.hpp`
  - `src/backend/prealloc/value_locations.hpp`
  - `src/backend/prealloc/publication_plans.hpp`
  - `src/backend/prealloc/publication_plans.cpp`
- Keep or clarify these shared facts:
  - `PreparedEdgePublicationLookups`
  - `PreparedMoveBundleLookups`
  - `find_indexed_prepared_edge_publications`
  - `find_unique_indexed_block_entry_parallel_copy_edge_publication`
  - `prepare_edge_copy_source_facts`
- Split or rename routing-oriented surfaces around:
  - current-block join parallel-copy source facts
  - instruction routing for those facts
  - same-width i32 stack-source publication
  - aggregate stack-source authority
- Known AArch64 consumers:
  - `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  - `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/memory.hpp`
  - `src/backend/mir/aarch64/codegen/prologue.cpp`

## Out Of Scope

- Deleting reusable edge publication, predecessor/successor, destination home,
  source producer, move-bundle, or parallel-copy authority facts.
- Moving AArch64 emission policy, scratch-register selection, final
  instruction forms, or register spelling into shared prealloc code.
- Rewriting out-of-SSA parallel-copy generation.
- Changing control-flow semantics or predecessor/successor transfer identity.

## Acceptance Criteria

- Shared APIs clearly distinguish target-neutral edge facts from target-facing
  routing conveniences.
- Any AArch64-specific routing helper is either target-local or named as a
  target-facing convenience, while reusable edge facts remain available for
  other backends.
- AArch64 edge-copy and memory paths continue to use prepared edge facts and do
  not perform predecessor rescans or local BIR/source rediscovery.
- The slice is bounded to edge-copy facade shape; it does not become a broad
  dispatch or memory lowering rewrite.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_' --output-on-failure`
- Escalate to full `ctest --test-dir build -j --output-on-failure` if edge
  publication semantics, parallel-copy authority, or control-flow transfer
  construction changes.

## Reviewer Reject Signals

- Reject if reusable edge publication or move-bundle queries are deleted and
  AArch64 consumers start rescanning predecessors, BIR producers, or local
  value locations.
- Reject if AArch64 register spelling, scratch policy, or final instruction
  emission moves into shared prealloc code.
- Reject if a named testcase shortcut is added for a specific edge-copy shape
  instead of clarifying fact/routing ownership.
- Reject if backend tests are weakened, edge-copy cases are marked
  unsupported, or expectation rewrites are used as proof.
- Reject if the exact old mixed facade remains but is renamed, especially
  around current-block join parallel-copy routing and typed stack-source
  publication.
