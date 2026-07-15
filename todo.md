# Current Packet

Status: Active
Source Idea Path: ideas/open/780_lir_cross_function_value_id_ownership_restoration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Diagnose cross-function native-ID ownership

## Just Finished

- Switched from 778 after its full-suite candidate for `b4685da80` was
  rejected (3037 total; 2883 passed; 154 failed) for cross-function foreign-ID
  verifier failures. No blocker implementation packet has run.

## Suggested Next

- Trace `fresh_value`, `LirFunction` ownership registration, and the
  standalone cast verifier across two functions; identify the narrow shared
  repair seam before editing code.

## Watchouts

- Keep the standalone verifier flag-gated and fail-closed. Do not special-case
  logical casts or absorb PHI/generic producer migration.

## Proof

- Diagnostic packet: record a reproducible multi-function ownership trace or
  focused reproduction. Later code packets require a fresh build and matching
  focused test; Step 4 requires a fresh full-suite candidate.
