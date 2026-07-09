Status: Active
Source Idea Path: ideas/open/615_branch_stack_source_residual_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Branch Stack-Source Residuals

# Current Packet

## Just Finished

Step 1, `Refresh Branch Stack-Source Residuals`, refreshed the current
branch stack-source residuals and compared them with the closed `590`, `592`,
`593`, `594`, and `596` contracts.

Current backend case logs still show `10` branch stack-source rows:

- `7` `unsupported_branch_stack_load_authority` rows:
  `src/20001017-1.c`, `src/loop-2e.c`, `src/pr39100.c`,
  `src/20000314-3.c`, `src/20140828-1.c`, `src/20080519-1.c`, and
  `src/20050125-1.c`.
  These rows are fused pointer `Lhs`/`Rhs` stack-load branch operands with
  `source_freshness_status=selected` and `source_freshness_candidates=1`, but
  their current first owner is
  `authority_status=missing_stack_clobber_safety`. They do not have complete
  prepared authority for an RV64 consumer repair under idea `615`.
- `3` `unsupported_branch_stack_load_source_freshness` rows:
  `src/930930-1.c`, `src/990127-1.c`, and `src/20060910-1.c`.
  These rows are fused pointer `Lhs`/`Rhs` stack-load branch operands with
  `authority_status=missing_source_freshness_authority`,
  `source_freshness_status=no_candidate`, and
  `source_freshness_candidates=0`. They are missing the selected
  `BranchStackLoadSource` / `BranchStackSlot` freshness required by the closed
  contracts and do not have complete prepared authority for RV64 consumption.

Contract comparison:

- Idea `590` requires branch-point freshness; stack homes and branch payloads
  are not authority by themselves.
- Idea `592` requires producer-published `BranchStackSlot` freshness for the
  exact branch block and terminator instruction point.
- Ideas `593` and `594` consume that selected freshness in RV64 for the fused
  pointer `Lhs` and `Rhs` stack-slot paths.
- Idea `596` clears the pointer `Rhs` producer policy path only when selected
  freshness can be published; it does not authorize target-local fallback.

`src/921124-1.c` is not currently a branch stack-source freshness row or a
select-publication row. Its current first owner is
`unsupported_terminator_fragment`, so it should remain outside idea `615`
implementation work.

## Suggested Next

Do not implement Step 2 from this audit. Hand off for a lifecycle split or
classification decision:

- split the `7` `missing_stack_clobber_safety` branch stack-load authority
  rows as a prepared clobber-safety publication/authority gap;
- split the `3` `missing_source_freshness_authority` / `no_candidate` rows as
  a prepared branch stack-source freshness publication gap; and
- leave `src/921124-1.c` with terminator lowering, not branch stack-source or
  select publication.

## Watchouts

- No current residual row has complete prepared/RV64 authority for a semantic
  RV64 branch stack-source consumer repair.
- Do not convert the `missing_stack_clobber_safety` rows into target-local
  stack-home acceptance; clobber safety is an authority requirement in the
  closed contracts.
- Do not merge the `no_candidate` rows with terminator lowering or select
  publication; they need producer freshness authority before consumption.
- Reject expectation or unsupported-marker downgrades as progress.

## Proof

Ran delegated proof exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests. Proof log: `test_after.log`.
