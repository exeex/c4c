Status: Active
Source Idea Path: ideas/open/610_rv64_move_bundle_target_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Tighten Diagnostics And Regression Boundaries

# Current Packet

## Just Finished

Completed `plan.md` Step 4 diagnostic/regression-boundary review for the
remaining idea 610 move-bundle residuals. No code, expectation,
unsupported-marker, allowlist, timeout/accounting, runtime, prepared/prealloc,
destination fan-in, ABI, terminator, or global-data files were changed.

The fresh Step 3 direct object evidence under
`build/agent_state/610_step3_move_bundle_residual_probe/summary.tsv` is
reviewably distinguished:
- 41 rows compile to RV64 objects.
- 1 remaining same-family RV64 consumer row is `src/pr71631.c`: an
  `out_of_ssa_parallel_copy` mixed multi-move target shape with complete
  published homes, but it includes repeated stack destinations and must remain
  fail-closed until destination fan-in/order authority exists.
- 10 rows reroute to the narrowed destination fan-in classifier
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`
  with `diagnostic_owner=rv64_prepared_move_bundle_consumer`.
- 13 rows are select-publication rows with explicit
  `select_publication_rejection_reason`: 7 unsupported stack-offset sources,
  4 unsupported source homes, and 2 unsupported immediate ranges.
- 2 rows are before-return ABI move-bundle rows (`src/20001130-2.c`,
  `src/20080719-1.c`), and 1 row is a separate call ABI failure
  (`src/20000808-1.c`).
- 14 rows are generic before-instruction stack-destination move shapes outside
  the out-of-SSA route: 12 multi-source stack-destination rows plus
  `src/920411-1.c` and `src/990829-1.c`.
- 5 rows are evidence gaps or malformed select-carrier facts:
  `src/pr47337.c` still reports the generic target-shape message without enough
  ownership facts, while `src/20080506-1.c`, `src/pr49186.c`,
  `src/pr68249.c`, and `src/pr78856.c` now report malformed/missing select
  materialization edge facts.
- 19 rows reroute to downstream non-move-bundle owners such as unsupported
  instruction fragments, local memory, terminators, scalar-compare
  publication, and inline asm.

Focused negative coverage already exists for the current boundaries:
`rejects_prepared_out_of_ssa_edge_preservation_fail_closed_shapes`,
`rejects_prepared_out_of_ssa_edge_preservation_stack_fail_closed_shapes`,
`rejects_prepared_out_of_ssa_phi_join_register_move_fail_closed_shapes`,
`rejects_prepared_out_of_ssa_phi_join_immediate_materialization_fail_closed_shapes`,
`reports_prepared_move_bundle_coordinate_diagnostic`,
`reports_prepared_select_publication_move_bundle_fragment_diagnostic`,
`rejects_ambiguous_non_parallel_multi_source_stack_destination_move_bundle`,
`rejects_prepared_before_return_stack_to_register_abi_move_fail_closed_shapes`,
and `rejects_prepared_before_return_fpr_abi_move_fail_closed_shapes`.

## Suggested Next

Move to `plan.md` Step 5 close-readiness review. Recommended decision:
continue or split, not close, unless the supervisor accepts leaving the single
same-family `src/pr71631.c` repeated-destination `out_of_ssa_parallel_copy`
consumer residual blocked outside idea 610. If continuing within idea 610, the
next narrow packet should be diagnostic/design-first for repeated destination
authority; do not force `src/pr71631.c` through without explicit fan-in/order
authority.

Exact proof command remains:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- Do not infer destination ordering or mutual exclusion from an encodable RV64
  move sequence. `src/pr71631.c` remains the live same-family audit row, but
  its repeated stack destinations are unresolved.
- The current diagnostics are reviewable enough to avoid a Step 4 code edit:
  select-publication, destination fan-in, ABI/before-return, generic
  before-instruction, malformed select-carrier/evidence-gap, and downstream
  non-move-bundle rows are distinguishable.
- `src/pr47337.c` is the only residual still lacking good ownership facts under
  the generic target-shape diagnostic; treat it as an evidence gap, not proof
  that a new RV64 materialization rule is safe.
- No test expectation, unsupported marker, allowlist, timeout/accounting, or
  runtime changes were made or needed for this packet.

## Proof

Proof command run for this Step 4 diagnostic-boundary packet:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: passed. Backend subset used: `^backend_`. Log path:
`test_after.log`. The proof is sufficient for this Step 4 diagnostic-boundary
slice.
