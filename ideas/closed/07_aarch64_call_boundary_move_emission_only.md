# AArch64 Call Boundary Move Emission Only

## Goal

Reduce AArch64 call-boundary move lowering to emission from complete prepared
facts.

## Why This Exists

`calls_moves.cpp` currently mixes target machine-node construction with source
selection, prior-preservation lookup, byval lane discovery, frame-slot address
recovery, and special materialization decisions. After prepared source
selection is complete, this file should only translate structured sources and
destinations into AArch64 machine instructions.

## In Scope

- Identify decision logic in `calls_moves.cpp` that should already be prepared
  facts.
- Replace target-local reconstruction with consumption of complete prepared
  records.
- Keep target-specific register view and instruction emission local.
- Preserve existing machine-node payloads and printer behavior.
- Add focused tests for emission from each complete source kind.

## Out Of Scope

- Changing prepared record ownership; that belongs to ideas `05` and `06`.
- Removing calls files before emission-only boundaries are stable.
- Broad dispatch cleanup.

## Acceptance Criteria

- `calls_moves.cpp` no longer decides which semantic call argument source wins.
- AArch64-specific code only builds operands, effects, and machine instructions
  from prepared facts.
- Representative c_testsuite and dispatch tests pass.
- Any remaining fallback is explicitly documented as temporary and not a broad
  prior-home rederivation path.

## Closure Note

Closed after Step 5 validation at `50f59b90a` (`Repair AArch64 prepared call
source fallbacks`). The Step 3 AArch64 prepared source-selection regression for
`c_testsuite_aarch64_backend_src_00216_c` and
`c_testsuite_aarch64_backend_src_00204_c` was repaired in-plan, and reviewer
coverage found no testcase-overfit or expectation weakening.

Validation accepted for closure:

- focused Step 5 repair proof passed 6/6
- `^c_testsuite_aarch64_backend_` passed 220/220
- `^backend_` passed 162/162
- backend regression guard passed with 162/162 before and after, no new
  failures
- accepted full-suite baseline passed 3410/3410 at `50f59b90a`

Known follow-up debt remains outside this closed idea: retire the documented
temporary absent-selection fallbacks once prepared facts are complete for local
aggregate address publication and indirect byval lane payloads.

## Reviewer Reject Signals

- A patch claims emission-only status while still scanning call plans or value
  homes to choose a source.
- A patch hides semantic decisions in printer/effect helpers.
- A patch removes coverage for source-selection edge cases.
- A patch changes runtime behavior without a prepared-fact explanation.
