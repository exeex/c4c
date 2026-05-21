# AArch64 Shift Promotion Codegen Scalability Timeout

Status: Open
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md

## Goal

Repair the backend-native AArch64 asm-generation timeout for the expanded
shift/type-promotion control-flow shape represented by `00200`, without
changing timeout policy or treating the stale generated binary's runtime
return-status symptom as the owner.

## Why This Exists

Umbrella idea 295 classified `00200` as a backend compile/codegen-time
timeout, not a generated-program runtime hang. The source is the
`lshift-type.c` promotion/left-shift macro stress case. Semantic and prepared
BIR for `main` contain a large straight-line expansion of nested
`do { ... } while (0)` checks whose conditions are constant false, and a
constant-false CFG walk reaches the final `printf`/`ret`.

The existing generated AArch64 binary exits quickly when run directly and
prints `0 test(s) failed`, so runtime loop behavior is not the observed
timeout mechanism. The focused owner is AArch64 backend scalability across
prepared lowering, register allocation, and code emission for the expanded
shift/type-promotion CFG.

## In Scope

- Localize the first backend-native phase where `00200` crosses the timeout
  window during AArch64 asm generation.
- Compare semantic BIR, prepared BIR, machine preparation, register allocation,
  and final emission behavior for the expanded shift/type-promotion CFG.
- Repair the general backend rule or data-structure behavior that causes
  pathological compile/codegen time on this straight-line constant-false
  promotion/shift expansion.
- Add focused backend coverage for the localized scalability shape without
  encoding the `00200` filename.
- Prove `c_testsuite_aarch64_backend_src_00200_c` advances beyond the backend
  asm-generation timeout or passes with the existing timeout policy.

## Out Of Scope

- Generated-program runtime loop fixes for `00200`.
- The stale generated binary's nonzero return-status symptom unless fresh
  evidence shows it is reached after the codegen timeout is repaired.
- Dynamic stack, VLA, fixed-slot, or moving-`sp` addressing work for `00207`.
- Timeout-limit changes, runner behavior, CTest registration, unsupported
  classifications, expectation rewrites, proof-log policy, or broad
  test-contract changes.
- Filename-specific shortcuts for `00200` or source-line-specific suppression
  of one macro expansion.

## Acceptance Criteria

- The first bad fact is localized to a concrete AArch64 backend phase and
  operation family, with evidence from bounded diagnostics.
- Focused backend coverage fails before the repair and passes after it for the
  localized shift/type-promotion scalability shape.
- `c_testsuite_aarch64_backend_src_00200_c` advances beyond asm generation
  timeout or passes under the existing runner and timeout policy.
- The repair preserves existing AArch64 scalar shift, type-promotion, compare,
  branch, and call-return behavior.
- Any remaining runtime return-status issue is explicitly classified as a
  separate follow-up rather than claimed as solved by compile-time progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00200`, `lshift-type.c`, one macro name, one source line, one
  constant, or one emitted instruction neighborhood instead of repairing a
  general backend shift/type-promotion codegen scalability rule;
- changes timeout policy, runner behavior, CTest registration, proof-log
  behavior, unsupported classifications, expectations, allowlists, or external
  test contracts;
- claims progress from pass-count movement, helper renames, classification
  notes, expectation changes, or count-only evidence while backend asm
  generation for the same shift/type-promotion CFG still times out;
- treats the stale generated binary's runtime return-status mismatch as the
  primary owner without fresh evidence that the backend-native timeout has
  already advanced;
- broadens into dynamic stack/VLA addressing, ABI composite work, semantic BIR
  admission, or unrelated runtime buckets without a separate lifecycle split.
