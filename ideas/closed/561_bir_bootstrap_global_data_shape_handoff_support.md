# BIR Bootstrap Global Data-Shape Handoff Support

Status: Closed
Type: Related BIR handoff implementation follow-up
Parent: `ideas/closed/545_bir_semantic_producer_admission_reconstruction.md`
Owning Layer: BIR bootstrap and global data-shape support

## Closure Note

Closed after the retained BIR bootstrap/global data-shape handoff boundary was
repaired for string-backed nested `char` array globals. Focused BIR coverage
now proves typed byte-string initializers lower through nested integer-array
recursion into coherent byte-addressable global storage facts.

The representative RV64 subset advanced through this source idea's handoff
boundary: `src/strlen-2.c` now exposes a separate `gep local-memory semantic
family` residual, while `src/20000703-1.c`, `src/20041218-1.c`, and
`src/20140212-1.c` remain downstream `unsupported_global_data` prepared/RV64
object-route work. Those residuals are outside this idea's scope.

Close-time regression guard passed for backend CTest: `345/345` before and
`345/345` after.

## Goal

Repair the `44` related BIR bootstrap/global data-shape handoff rows without
counting them as exact `semantic lir_to_bir` producer rows.

## Why This Exists

The evidence reconstruction found `44` related rows with BIR handoff failures
around scalar integer/pointer globals, linear integer-array globals, and
aggregate-backed globals with byte-address semantics. They are adjacent to BIR
semantic admission but intentionally excluded from the `373` exact semantic
producer row set.

## In Scope

- Inspect BIR bootstrap/global data-shape handling before prepared object
  handoff.
- Add focused tests for scalar pointer/integer globals, linear integer-array
  globals, and aggregate-backed globals with byte-address semantics.
- Repair the handoff limitation without merging these rows into local-memory
  semantic producer work.
- Prove with a narrow RV64 subset drawn from the `44` related rows, including
  `src/strlen-2.c` or a current stronger representative.

## Out Of Scope

- Counting these rows as exact `semantic lir_to_bir` rows.
- Local-memory, call metadata, runtime/intrinsic, or scalar/signature/control
  semantic producer repairs.
- Prepared/RV64 ownership claims before the BIR handoff limitation is removed.
- Expectation rewrites, unsupported downgrades, allowlist changes, or weaker
  handoff checks.

## Acceptance Criteria

- The bootstrap/global data-shape limitation is repaired or split into clearer
  source ideas if the inspected rows do not share one handoff boundary.
- Focused tests cover the supported global data shapes.
- A representative RV64 subset advances through the BIR handoff without
  downstream inference.
- The exact semantic producer row counts remain distinct from this related
  lane.

## Reviewer Reject Signals

- Reject counting these `44` rows as part of the `373` exact semantic producer
  rows.
- Reject merging bootstrap/global data-shape support into local-memory
  semantic producer work without fresh row evidence of a shared boundary.
- Reject prepared or RV64 repairs that bypass the BIR handoff limitation.
- Reject expectation or unsupported-marker downgrades as progress.
- Reject helper renames that retain the same global data-shape handoff failure.
