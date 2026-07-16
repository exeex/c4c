# LIR Typed Reference Carriers and Collector Migration

Status: Open
Type: first-owner semantic reference-carrier migration
Matrix Rows: M14
Dependencies: 839, 843, 844; preserves 734; coordinates 812/813

## Goal

Replace each raw-text symbol scanner only when the exact producer publishes a
semantic callee, argument, signature, or global reference carrier.

## In Scope

- Migrate named call/global collectors and LIR-to-BIR preparation one source
  field at a time, with compatibility parity.

## Out Of Scope

- Reconstructing references from rendering, ownership of residual non-type
  strings, or 734 receiver/type-model repair.

## Acceptance Criteria

- Fresh build plus call/global collector and LIR-to-BIR proof for every
  replaced field, with no missing or spurious references against compatibility.
- Delete a scanner only after its exact source field has a semantic carrier;
  queue residual non-type text to 812 then 813.

## Reviewer Reject Signals

- Reject rendered-text/name scans renamed as carriers, broad collector sweeps,
  missing/spurious-reference tolerance, or claims to complete 734/812/813.
