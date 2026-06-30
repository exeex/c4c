Status: Active
Source Idea Path: ideas/open/454_edge_dependency_operand_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Dependency-Operand Authority Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 454. The contract artifact is
`build/agent_state/454_step2_dependency_operand_authority_contract/contract.md`.

Accepted shapes:

- Shared requirements: edge identity, source-producer identity, dependency
  operand identity/role, value id/name/type agreement, predecessor-terminator
  placement, and available register destination publication.
- `load_from_stack_slot`: accepted only with stack value-home/object linkage,
  coherent slot id/offset/size/alignment/pointer width, edge-safe freshness,
  clobber-safety, and explicit load policy.
- `rematerialize_cast_from_source`: accepted only with explicit cast producer,
  cast result matching the dependency operand, explicit source identity,
  target-consumable source home, supported cast kind, and supported pointer
  width semantics.

Rejected shapes:

- stack home plus object metadata without policy;
- raw `inttoptr` plus immediate source without policy;
- successor/join-block result copies such as `%t18`;
- dependency records not tied to a specific edge and source producer;
- missing/mismatched value id/name/type or missing stack object linkage;
- missing freshness/clobber-safety for stack loads;
- missing cast source, unsupported cast kind/type/width, or non-consumable cast
  source home;
- non-predecessor-terminator placement, unavailable/non-register destination,
  pointer-value provenance, generic stack-home branch work, or RV64 lowering.

Step 2 conclusion: this producer layer can own the policy representation and
operand linkage contract. Freshness/clobber-safety remains required for
`load_from_stack_slot`; if Step 3 cannot express that fact, it should keep
stack-load authority fail-closed and either implement only a coherent explicit
cast-rematerialization metadata path or record a metadata blocker.

## Suggested Next

Step 3: `Add Dependency-Operand Authority Metadata Coverage`.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` if route lookup
  helper coverage is clearer
- `todo.md`
- `test_after.log`
- `build/agent_state/454_step3_dependency_operand_authority_metadata/*`

Add the smallest prepared metadata type/status enum/planner or predicate for
dependency operands of edge source producers. Cover accepted explicit
`rematerialize_cast_from_source` if current producer facts can express the cast
relationship and width policy. Cover `load_from_stack_slot` only if
freshness/clobber-safety can be expressed; otherwise keep it fail-closed with a
precise status.

Proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 target lowering from current facts.
- Do not treat `%t17` stack home plus object metadata as sufficient
  `load_from_stack_slot` authority.
- Do not treat raw `inttoptr` plus `%t16` immediate as sufficient
  `rematerialize_cast_from_source` authority.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 contract proof:

```sh
git diff --check
```

Result: passed.
