# Select Publication Source Wiring

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/598_select_carrier_alias_freshness_contract.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/RV64 authority
Queue Order: 15
Prerequisites: preserve the idea `589` and `598` distinction between source freshness, alias evidence, and destination legality
Estimated Evidence Breadth: `11` select publication stack-offset and move-bundle wiring rows
Proof Surface: select publication rows that reject stack-offset source or move-bundle wiring despite existing publication evidence

## Goal

Wire select publication source evidence into the prepared/RV64 boundary for
supported stack-offset and move-bundle rows without treating alias evidence or
destination legality as source freshness.

## Why This Exists

The current map shows `7` select publication stack-offset source rows and `4`
select publication move-bundle wiring rows that look close after ideas `589`
and `598`.

## In Scope

- Select publication source wiring where freshness is already established.
- Accurate rejection for rows missing source freshness or destination legality.
- Proof across both stack-offset and move-bundle select publication rows when
  available.

## Out Of Scope

- General select instruction lowering.
- Destination fan-in authority.
- Branch stack-source, pointer local-memory, ABI, runtime, expectations,
  unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple select publication rows progress when source freshness exists.
- Rows relying only on alias evidence or destination legality remain rejected.
- Proof demonstrates the source-freshness boundary from ideas `589` and `598`.

## Closure Notes

Closed after Step 4 classified the active runbook as complete for idea `616`.
Step 2 wired a bounded prepared/RV64 select-publication source path and moved
multiple complete-authority stack-offset select-publication rows past
`unsupported_source_stack_offset`, while keeping rows without explicit source
freshness rejected. The delegated backend proof passed with:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

The closure boundary remains the idea `589`/`598` contract: source freshness,
alias evidence, and destination legality are separate facts. Alias evidence or
destination legality alone was not accepted as source freshness.

Residual rows are separate owners, not blockers for this idea:

- `src/20000706-1.c`, `src/20000706-2.c`, `src/20000717-5.c`,
  `src/20071213-1.c`, `src/20120427-1.c`, and `src/20120427-2.c` now reach
  `[RV64_BACKEND_RUNTIME_MISMATCH]`; separate runtime/semantic owner.
- `src/991216-1.c` now fails first on generic
  `unsupported_move_bundle_target_shape`; separate generic move-bundle
  target-shape owner.
- `src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, and `src/pr59221.c`
  remain `intent_status_unsupported_source_home`; separate source-home/source
  freshness owner.
- `src/pr29695-1.c` and `src/pr29695-2.c` remain
  `unsupported_source_immediate_i32_range`; separate large-immediate
  materialization/range owner.
- `src/921124-1.c` and `src/920710-1.c` remain
  `unsupported_terminator_fragment`; separate terminator owner.

Close-time regression guard used existing backend-scope `test_before.log`,
regenerated matching `test_after.log`, and passed the monotonic checker with
`--allow-non-decreasing-passed` because closure itself is lifecycle-only and
does not change implementation behavior.

## Split-In From Idea 611

Idea `611` close-readiness classified `src/921124-1.c` and `src/920710-1.c`
as terminator-labeled direct-object residuals whose prepared evidence points
through join/select carrier or predecessor-terminator parallel-copy authority.
Treat these as candidate select publication rows only after refreshed
diagnostics prove source freshness and destination legality under the idea
`589`/`598` boundary.

## Reviewer Reject Signals

- Reject treating alias evidence as source freshness.
- Reject destination legality shortcuts inside select publication wiring.
- Reject named-case-only fixes such as a single stack-offset row.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes.
- Reject retaining `unsupported_source_stack_offset` behind renamed code.
