# 144 Source-producer and same-block materialization owner

## Goal

Move source-producer and same-block materialization declarations out of the
residual prepared lookup facade into a narrow source-producer/select-chain
owner.

## Why This Exists

Idea 141 found that same-block producer query facts, publication
source-producer lookup declarations, current-block publication consumption, and
same-block scalar/integer-constant query helpers are semantic
source-producer/materialization facts. Their implementation already overlaps
with `select_chain_lookups.cpp` and publication/source-producer ownership.

## In Scope

- Move same-block producer query facts.
- Move the publication source-producer lookup helper declaration toward
  `src/backend/prealloc/select_chain_lookups.hpp`.
- Move current-block publication consumption query declarations.
- Move same-block scalar and integer-constant query helper declarations.
- Use `src/backend/prealloc/publication_plans.hpp` only for fact structs that
  already belong to publication planning.

## Out Of Scope

- Reopening completed select-chain ownership from idea 137 except for this
  concrete residual declaration dependency.
- Moving AArch64 materialization, register allocation, scratch, hazard, or final
  emission policy into prealloc.
- Replacing prepared source-producer facts with local target scans.
- Folding call-argument, comparison, or load-local specialized policies into
  this route before the shared same-block owner is stable.

## Acceptance Criteria

- Source-producer and same-block materialization APIs have a narrower public
  owner than `prepared_lookups.hpp`.
- Consumers no longer depend on the broad facade for these declarations unless
  they still need `PreparedFunctionLookups`.
- Proof includes `cmake --build --preset default` and
  `ctest --test-dir build -R '^backend_' --output-on-failure`.
- Escalate to full CTest if source-producer semantics change.

## Reviewer Reject Signals

- The new header hides the old facade without clarifying source-producer or
  same-block ownership.
- AArch64 consumers are rewritten around local rediscovery.
- The implementation expands into call, comparison, or memory policy before the
  shared same-block API boundary is stable.

## Closure Note

Closed after implementation commit `b30d4ea7a`. The same-block/source-producer
APIs now have the narrow public owner, consumers were updated where they directly
use those declarations, and `prepared_lookups.hpp` remains only as aggregate
wiring compatibility where residual prepared lookup declarations still require
the moved types by value.

Close proof used the executor build plus backend CTest logs:
`cmake --build --preset default` and
`ctest --test-dir build -R '^backend_' --output-on-failure`.
Close-time regression guard passed with `test_before.log` and `test_after.log`:
179 passed / 0 failed / 179 total before and after.
