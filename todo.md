Status: Active
Source Idea Path: ideas/open/233_phase_e3_route7_materialized_condition_helper_oracle_follow_up.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Use Route 7 Evidence Only Under Prepared Agreement

# Current Packet

## Just Finished

Completed `plan.md` Step 1 discovery for the selected materialized-condition
helper-oracle row without changing code or tests.

Exact row:

- File: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.
- Helper: `verify_prepared_bir_comparison_condition_producer_equivalence()`.
- Row: the materialized-condition block at lines 10429-10528 that checks
  `%cond` at `block.insts.size()` after the fused-compare agreement-boundary
  checks.
- Positive assertion shape: `prepared_condition` from
  `prepare::find_prepared_materialized_condition_producer(...)`,
  `bir_condition` from `bir::find_materialized_condition_producer_identity(...)`,
  direct `route7_condition` from
  `bir::route7_comparison_instruction_record(&block, 3)`,
  indexed `route7_condition` from
  `bir::route7_find_materialized_condition(route7_index, block, %cond,
  block.insts.size())`, and `route7_condition_ref` from
  `bir::route_index_validate_materialized_condition_reference(...)` all agree on
  the same binary producer, instruction index, condition value, and lhs/rhs
  producer facts.
- Existing fallback assertion shape: absent Route 7 evidence uses an empty
  `Route7ComparisonConditionIndex` and empty `RouteIndexReferenceFacade`, while
  preserving `prepared_condition` and `bir_condition` availability and requiring
  `MissingBlock`/`MissingRecord`.

Route 7/prepared agreement boundary:

- Route 7 evidence starts at
  `src/backend/bir/bir.cpp:3991`
  `route7_find_materialized_condition(...)` and the facade validator at
  `src/backend/bir/bir.cpp:4876`
  `route_index_validate_materialized_condition_reference(...)`.
- BIR identity is currently derived from the Route 7 index/reference facade in
  `src/backend/bir/bir.cpp:4993`
  `find_materialized_condition_producer_identity(...)`.
- Prepared authority is
  `src/backend/prealloc/comparison.cpp:108`
  `find_prepared_materialized_condition_producer(...)`, which only accepts a
  named condition whose same-block scalar producer is a prepared binary.
- Production AArch64 already has the intended fail-closed boundary in
  `src/backend/mir/aarch64/codegen/comparison.cpp`: valid Route 7 evidence is
  normalized by
  `find_valid_route7_materialized_condition_producer_identity(...)`, then used
  only when it matches BIR identity and agrees with
  `find_prepared_materialized_condition_producer_identity(...)`; otherwise the
  selected producer remains the BIR/prepared fallback path.

## Suggested Next

Delegate Step 2 as a helper-oracle-only implementation packet.

Minimal Step 2 target files:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` for the selected
  row and any local helper lambdas needed to assert Route 7/prepared agreement
  without changing helper-oracle strings.
- Potentially `src/backend/mir/aarch64/codegen/comparison.cpp` only if the next
  packet needs to expose/reuse the existing materialized-condition agreement
  predicate for testing; avoid touching it if the row can assert through
  existing public BIR/prepared helpers.

Positive proof case:

- Extend the selected row to prove Route 7 materialized-condition evidence is
  accepted only when `route7_find_materialized_condition(...)`,
  `route_index_validate_materialized_condition_reference(...)`,
  `find_materialized_condition_producer_identity(...)`, and
  `find_prepared_materialized_condition_producer(...)` agree on `%cond`, binary
  producer pointer, instruction index `3`, prepared condition value name, and
  lhs/rhs producer facts.

Fallback proof cases to add or tighten in the selected row:

- Absent Route 7 authority: already present, keep prepared/BIR facts intact.
- Invalid reference: mutate a Route 7 comparison record to a stale/null
  instruction or stale owner and require no Route 7-backed agreement.
- Duplicate/conflict: duplicate a `%cond` comparison record and require
  `DuplicateReference`/`DuplicateProducer` behavior with prepared authority
  retained.
- Route/prepared mismatch: alter prepared/source-producer or Route 7/BIR
  condition identity so agreement fails without weakening the prepared oracle.
- Unfused/non-comparison path: keep materialized-condition lookup unavailable
  for non-comparison or non-condition rows.
- Prepared fallback/non-agreement: partial lhs/rhs producer mismatch or missing
  operand evidence must not become Route 7-backed success.

## Watchouts

Nearby same-feature cases already available for Step 2/3 proof selection:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_bir_comparison_condition_producer_identity_lookup()` covers Route 7
  materialized-condition records, reference facade validation, stale owner,
  wrong role, duplicate operand reference, missing condition, non-comparison,
  and missing producer behavior.
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  `verify_production_bir_comparison_condition_producer_equivalence()` covers a
  production prepared/BIR materialized-condition row, but not Route 7 materialized
  condition agreement.
- `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`
  `materialized_compare_branch_route7_provenance_matches_bir_identity()` covers
  positive Route 7/BIR/prepared provenance plus unchanged branch-control,
  machine-printer payload, and assembler oracle output.
- Nearby branch-control fallbacks cover duplicate Route 7 provenance, absent
  Route 7 provenance, condition-name mismatch, invalid Route 7 reference, stale
  prepared lookup, lhs provenance mismatch, and rhs provenance mismatch.

No-change surfaces for Step 2:

- Do not change `plan.md`, the source idea, implementation behavior, expected
  strings, helper-oracle strings, baselines, branch-control output,
  machine-printer output, wrappers, final assembler behavior, route-index public
  contracts, supported/unsupported contracts, suffix mapping, fused legality,
  hazard checks, or emitted-output authority.
- Do not add fixture-name, row-name, or named-case shortcuts. The row must prove
  semantic agreement through Route 7 materialized-condition evidence and prepared
  facts.

Step 2 proof gaps:

- The selected helper row currently has only positive agreement and absent-route
  fallback for materialized-condition evidence.
- It does not yet assert invalid-reference, duplicate/conflict, route/prepared
  mismatch, unfused/non-comparison, or partial non-agreement fallback in that
  row.
- Existing branch-control tests prove production behavior, but the helper-oracle
  row still needs local assertions so Step 2 is not justified solely by target
  output.

## Proof

 Passed delegated proof:

- `git diff --check -- todo.md && git status --short`
- Output showed only `M todo.md`.
