# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Create Follow-Up Ideas Only For Concrete Candidates

## Just Finished

Completed Step 4, "Create Follow-Up Ideas Only For Concrete Candidates", by
creating follow-up ideas for the two Step 3 candidates with traced record
paths, bounded owner boundaries, focused proof routes, non-goals, and concrete
reviewer reject signals.

Created ideas:

- `ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md` for the
  local aggregate helper/table contraction candidate around load mnemonics,
  printable chunk selection, chunk printability, destination-lane derivation,
  aggregate lane memory, and scratch exclusion in `instruction.cpp`.
- `ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md`
  for the call-boundary/aggregate-lane record schema cleanup candidate around
  `CallBoundaryMoveInstructionRecord`, generic source/destination/provenance
  fields, and `is_aggregate_register_lane_publication`.

No follow-up idea was created for target-local ABI/printer ownership surfaces
because Step 3 evidence says those surfaces own AArch64 ABI phase, diagnostics,
scratch choices, selected-node facts, final printable assembly constraints, or
printer-owned rejection behavior.

No follow-up idea was created for missing-evidence surfaces, including
`CallBoundaryAbiBindingInstructionRecord`, broad surface identifiers/public
printer entry points, and frame-slot load/address materialization checks split
between selection status and printing. Step 3 did not prove stable
no-semantics helper or schema boundaries for those areas.

## Suggested Next

Execute Step 5, "Prepare Audit Close Summary", by summarizing the audited
ownership surfaces, the two created follow-up ideas, the rejected
target-local/missing-evidence surfaces, and the audit-only proof result.

## Watchouts

- This is audit-only; do not edit implementation files while executing the
  current plan.
- Do not claim printer cleanup by deleting validation or moving ABI-specific
  call-boundary construction into shared authority.
- Preserve printer-owned diagnostics and scratch decisions even for surfaces
  classified as cleanup candidates.
- Do not turn `missing-evidence` areas into Step 4 implementation ideas.
- The two concrete candidates already became follow-up ideas 90 and 91; do not
  create additional ideas for Step 3 target-local ownership or missing-evidence
  areas.
- Stack-lane inline-asm publication remains aggregate-specific construction
  ownership and should not be folded into register-lane printer cleanup.
- Any Step 5 close summary should preserve that no backend tests were required
  because only lifecycle/audit artifacts changed.

## Proof

Audit-only proof command for Step 4:

```sh
printf 'Audit-only Step 4; no backend tests required.\n' > test_after.log && { git diff --name-only; git ls-files --others --exclude-standard; } >> test_after.log && if { git diff --name-only; git ls-files --others --exclude-standard; } | rg -q '^src/backend/mir/aarch64/codegen/(calls|instruction|machine_printer)\.(cpp|hpp)$|^plan\.md$'; then printf 'ERROR: implementation or plan file changed during audit-only follow-up creation packet.\n' >> test_after.log; exit 1; fi
```

Result: passed; `test_after.log` contains the audit-only Step 4 marker and
dirty file list. No implementation files or `plan.md` changed.
