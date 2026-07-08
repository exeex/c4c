# Select Publication Source Wiring

Status: Open
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
