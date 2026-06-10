Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 1 Oracle Coverage

# Current Packet

## Just Finished

Completed Step 1: inventoried Route 1 producer, constant, and materialization
consumer candidates and selected one bounded first implementation family.

Selected consumer family: AArch64 GP scalar value-publication materialization in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`.

Target files for the first code packet:
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` only if an oracle
  gap appears while adding or confirming coverage

Selected prepared consumers:
- `prepared_same_block_scalar_producer(...)` currently builds a
  `PreparedSameBlockValueMaterializationQuery` and calls
  `prepare::find_prepared_same_block_scalar_producer(...)`.
- `prepared_same_block_integer_constant(...)` preserves immediate fast-path
  behavior, then calls `prepare::evaluate_prepared_same_block_integer_constant(...)`.

Route 1 replacement surface:
- `mir::find_same_block_scalar_producer(...)` in
  `src/backend/mir/query.cpp`, already backed by `Route1ProducerIndex`.
- `mir::evaluate_same_block_integer_constant(...)` in
  `src/backend/mir/query.cpp`, already backed by
  `bir::route1_evaluate_same_block_integer_constant(...)`.

Rejected candidates:
- `src/backend/mir/aarch64/codegen/calls.cpp` scalar call-argument
  source-producer materialization: already has a Route 6 BIR path and falls back
  to prepared answers; it is call ABI and carrier-policy-adjacent, so it is not
  the smallest Route 1 first packet.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` prepared
  producer usage for FP global loads: mixed with prepared memory access,
  address policy, GOT/direct relocation, and FPR materialization choices.
- `src/backend/mir/aarch64/codegen/alu.cpp` and
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` control/publication
  producer helpers: tied to register movement, current-block join copy
  handling, and publication routing rather than pure Route 1 facts.
- `src/backend/prealloc/comparison.cpp` and comparison/fused branch consumers:
  Route 7 owns the relevant comparison-condition migration and branch-policy
  boundaries; not a Route 1 first packet.
- `PreparedFunctionLookups::edge_publication_source_producers` aggregate
  exposure in `src/backend/prealloc/prepared_lookups.hpp`: aggregate-only
  public surface, not safe to contract before at least one direct consumer
  moves.
- `find_prepared_same_block_global_load_access(...)` and
  `find_prepared_same_block_load_local_stored_value_source(...)`: Route 3
  memory/access semantics and target addressing policy, not pure Route 1.

Explicit negative cases to preserve before implementation:
- Missing named value has no producer and remains fail-closed.
- Producer after `before_instruction_index` remains invisible.
- Mismatched value type remains fail-closed.
- Non-integer or non-constant producer kinds do not materialize as integer
  constants.
- Non-materializable producer kinds still do not claim scalar materialization.
- Null or missing block context remains fail-closed without falling back to a
  broad prepared lookup rebuild.

## Suggested Next

Execute Step 2 for the selected AArch64 GP scalar value-publication
materialization family: confirm or add oracle coverage that compares prepared
same-block producer and integer-constant answers with the Route 1-backed MIR
helpers, then keep the implementation packet scoped to switching
`dispatch_value_materialization.cpp` from prepared same-block producer/constant
queries to `mir::find_same_block_scalar_producer(...)` and
`mir::evaluate_same_block_integer_constant(...)`.

## Watchouts

- Keep prepared producer and constant helpers visible as migration oracles until
  the selected consumer no longer needs them.
- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Reject testcase-shaped shortcuts, expectation rewrites, or narrow named-case
  matching as progress.
- The selected file still legitimately uses prepared homes, prepared memory
  accesses, local-load offsets, global-load emission policy, select-chain
  materialization, and publication helpers. The first implementation packet
  should replace only the same-block producer and integer-constant discovery
  source.
- Keep immediate values as a direct fast path; Route 1 migration is for named
  same-block lookup consumers, not for changing immediate materialization
  spelling.
- Do not contract `PreparedFunctionLookups` or
  `edge_publication_source_producers` in the first implementation packet.

## Proof

Not run; this packet was inventory-only and the delegated proof explicitly said
no build was required.

Recommended future proof command for the selected implementation packet:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'`

No `test_after.log` was produced for this inventory-only packet.
