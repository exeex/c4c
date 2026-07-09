Status: Active
Source Idea Path: ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Revise Producer Authority Family Selection

# Current Packet

## Just Finished

Reviewed the Step 2 family-revision evidence after the route review in
`review/647_step2_family_revision_review.md`. Fresh residual classification
and excerpts remain available under
`build/agent_state/647_step2_family_revision/`, with the summary in
`build/agent_state/647_step2_family_revision/summary.md`, but the previously
selected `select_materialization_preserved_stack_fallback` route is rejected
for idea 647 because it reopens the closed idea 637
`SelectMaterializationPreservedStackFallback` contract.

Rejected route: `src/20021204-1.c` at `main:tern.end.12` before instruction 1
still has the failing `%t20/%t21 -> %t22` register-source stack-destination
bundle with `authority=none`, `parallel_copy=no`, and
`fragment_status=producer_authority_missing_for_register_fan_in_stack_destination`.
The nearby `%t17/%t24 -> %t25` select edge/control-flow facts are unrelated to
the failing destination `%t22` and must not be reused as authority.

Rejected revised route: `src/20011109-2.c`, function `main`, block `block_1`,
before instruction 9 has real select-materialized preserved-stack-fallback
evidence, including select-chain evidence naming `%t12.sel1` as
`source_producer=select_materialization` at `block_1`, `root_inst=9`. That
evidence is useful diagnostic input, but it is not a legal Step 3 target under
idea 647 because the selected family would be the closed idea 637
`PreparedMoveAuthorityKind::StackDestinationRegisterFanIn` plus
`PreparedStackDestinationFanInSemantics::SelectMaterializationPreservedStackFallback`
contract. Keep this row outside idea 647's implementation route unless the
supervisor creates or switches to a separate lifecycle initiative for the
possible 637 closure gap.

Current Step 2 state: no legal revised first packet is selected yet under idea
647. Continue Step 2 only on non-637 families, such as ordered final-state
authority, mutual-exclusion authority supported by new producer evidence, merge
authority distinct from the idea 637 select-materialized contract, or another
explicitly named prepared/prealloc destination authority outside the closed
637 family.

Minimal negative examples:

- `src/20021204-1.c`: rejected mutual-exclusion row; no predicate, edge,
  selected-active-candidate, guarded-copy, or carrier fact for `%t22`.
- `src/920429-1.c`: failing row is `main:entry` before instruction 8, two
  register sources into stack destination `21`; select-carrier rows belong to
  function `f` `%t12` and report `unsupported_publication`.
- `src/930429-1.c` and `src/ptr-arith-1.c`: same-entry two-register fan-in
  rows with no matching select-chain or ordered-final-state authority at the
  consumer point.
- `src/pr34415.c` and `src/pr70005.c`: nearby join/select authority context
  belongs to other join results and reports `missing_carrier_aliases` or
  `unsupported_publication`, not authority for the failing consumer bundles.

Residuals left out of scope: plain ordered final-state authority,
mutual-exclusion authority, merge authority, the rejected
`src/20011109-2.c` select-materialized preserved-stack-fallback row, and all
other non-selected residual rows. They remain fail-closed under their current
missing-authority diagnostics until a future Step 2 packet or separate
lifecycle state proves a concrete non-637 producer fact for them.

## Suggested Next

Do not execute Step 3 yet. Delegate another Step 2 classification packet if the
supervisor wants to continue idea 647: it must select a first implementation
family outside the closed idea 637 select-materialized preserved-stack-fallback
contract, name minimal positive and negative examples, and identify the
prepared/prealloc fact shape, owner label, negative statuses, and consumer
program point. If no such non-637 family can be proven from the refreshed
residual evidence, record that no legal first packet is currently available
under idea 647 and ask the supervisor whether to split or switch lifecycle
state.

## Watchouts

- Do not reselect `src/20021204-1.c` for mutual exclusion; it remains rejected
  for this route.
- Do not reselect `src/20011109-2.c` as a Step 3 target through
  `SelectMaterializationPreservedStackFallback`; that is the closed idea 637
  contract, not idea 647 progress.
- Do not infer authority from move order, source freshness, source
  availability, final assembly, testcase identity, or unrelated select/join
  facts.
- Any selected family must be ordered-final-state, mutual-exclusion, merge, or
  another explicitly named non-637 destination-authority family proven by
  producer evidence at the consumer point. Rows lacking that proof must stay
  fail-closed.

## Proof

No build/ctest proof was required because this packet made no implementation
or test changes. Did not create or overwrite `test_after.log`. Lifecycle repair
was limited to `todo.md`.

Commands/evidence:

- For each residual target, ran `build/c4cll -I tests/c/external/gcc_torture
  --target riscv64-linux-gnu --dump-prepared-bir ...` and `--dump-bir ...`;
  all dump commands exited `0`.
- For each residual target, ran
  `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`; all
  object probes exited `1` with current fail-closed diagnostics.
- Durable evidence and excerpts are under
  `build/agent_state/647_step2_family_revision/`; summary at
  `build/agent_state/647_step2_family_revision/summary.md`.
