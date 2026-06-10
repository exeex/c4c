Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 1 Oracle Coverage

# Current Packet

## Just Finished

Completed Step 2: confirmed and extended Route 1 oracle coverage for the
selected AArch64 GP scalar value-publication materialization facts.

Existing coverage in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` already compares:
- `prepare::find_prepared_same_block_scalar_producer(...)`
  with `mir::find_same_block_scalar_producer(...)` and
  `bir::route1_find_same_block_scalar_producer(...)` across binary, cast,
  load-local, load-global, select-materialization, and product producers.
- `prepare::evaluate_prepared_same_block_integer_constant(...)`
  with `mir::evaluate_same_block_integer_constant(...)` and
  `bir::route1_evaluate_same_block_integer_constant(...)` for immediate and
  folded binary constants.
- materialization availability with direct Route 1 answers.
- fail-closed behavior for missing values, future producers, non-constant
  producer kinds, non-materializable producer kinds, and mismatched prepared
  producer facts.

Added one explicit oracle gap:
- mismatched value type now checks integer-constant equivalence fails closed
  for both prepared-vs-MIR and prepared-vs-direct-Route-1 paths.

## Suggested Next

Execute Step 3: migrate only the selected AArch64 GP scalar
value-publication materialization consumer in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` from
prepared same-block producer and named integer-constant discovery to
`mir::find_same_block_scalar_producer(...)` and
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
- Step 2 touched only oracle coverage; the selected consumer has not been
  switched yet.

## Proof

Ran the supervisor-selected proof command:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Result: passed. `backend_prepared_lookup_helper` and
`backend_aarch64_instruction_dispatch` both passed.

Proof log: `test_after.log`.
