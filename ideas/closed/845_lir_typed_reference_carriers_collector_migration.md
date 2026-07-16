# LIR Typed Reference Carriers and Collector Migration

Status: Closed
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

## 866 Reconciliation And 734 Return

Idea 866 orders this after producer/schema routes publish exact semantic
carriers. It is not first owner for missing facts. Return to 734 only when a
carrier migration accepts an exact field that a bounded Raw-BIR receiver row can
consume without scanner or rendered-text recovery.

## Reviewer Reject Signals

- Reject rendered-text/name scans renamed as carriers, broad collector sweeps,
  missing/spurious-reference tolerance, or claims to complete 734/812/813.

## Closure Disposition

Closed after supervisor-accepted implementation commits migrated the bounded
carrier-backed collector route one field at a time through
`LirShuffleVectorOp.vec2`:

- `ab3b775b9`: `LirExtractValueOp.agg`
- `58143987e`: `LirExtractElementOp.vec`
- `a54a42f14`: `LirInsertElementOp.elem`
- `c67b0fdd1`: `LirInsertElementOp.vec`
- `8c57acace`: `LirInsertValueOp.agg`
- `a067e8cc5`: `LirInsertValueOp.elem`
- `75100a96f`: `LirShuffleVectorOp.vec1`
- `068f0bc28`: `LirShuffleVectorOp.vec2`

Together with the earlier accepted route, the selected exact typed aggregate
and vector reference carriers now use semantic collector data instead of raw
rendered-text scanning. The accepted proof included fresh build and focused
collector test coverage for replaced fields, `git diff --check`, and an
accepted full-suite baseline candidate at `a067e8cc5`.

Residual raw paths, including alloca count, inline asm args, abs arg,
va/memcpy/stack/indirectbr, and shuffle mask scanning, are not claimed by this
closure. They remain residual or legacy scanner work for the existing 812/813
ownership path rather than scope expansion of this idea.
