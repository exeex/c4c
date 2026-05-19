# AArch64 C-Testsuite 00205 Value Materialization Residual

Status: Open
Created: 2026-05-19
Split From: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md

## Goal

Classify and repair the AArch64 generated-code value-materialization or
stack-frame cause of the now-reachable
`c_testsuite_aarch64_backend_src_00205_c` output mismatch.

## Why This Exists

Idea 304 repaired the focused timeout mechanism. The accepted post-timeout
state has `00064` and `00139` passing, while `00205` completes quickly and
fails output comparison instead of timing out.

The current evidence points away from branch/compare timeout behavior and
toward value materialization or stack-frame addressing: generated `00205`
assembly reads case fields from high stack offsets such as `[sp, #632]`,
`[sp, #1064]`, and `[sp, #1496]` despite a prologue that reserves only
`sub sp, sp, #48`. The residual must be handled by its own owner so the
timeout repair is preserved without silently expanding idea 304.

## In Scope

- Generated AArch64 code inspection for the `00205` output mismatch after the
  timeout repair.
- Stack-frame layout, alloca/local aggregate materialization, address
  computation, spill/reload, and value-copy semantics that can explain reads
  outside the reserved frame.
- Narrow probes that compare the generated assembly against the source
  testcase's case aggregate layout and runtime output.
- A semantic backend repair for the classified value-materialization or frame
  addressing bug.
- Focused proof using the supervisor-selected `00064`/`00139`/`00205`
  c-testsuite subset, preserving the legal sign-extension and repaired
  loop-bound compare behavior from ideas 303 and 304.

## Out Of Scope

- Reopening idea 304's branch/compare timeout route unless `00205` starts
  timing out again for the same loop-bound mechanism.
- Reopening idea 303's sign-extension spelling route unless generated assembly
  regresses to illegal `sxtw` destination width.
- Expectation rewrites, allowlist changes, unsupported classifications,
  timeout policy changes, runner behavior changes, proof-log policy changes,
  or CTest registration changes.
- Filename-only fixes or source-testcase-shaped shortcuts that make `00205`
  print expected output without repairing the underlying frame or value
  semantics.
- Broad ABI, stack-frame, aggregate, or instruction-selection rewrites before
  the concrete mismatch mechanism is classified.

## Acceptance Criteria

- The output mismatch mechanism is classified from generated assembly,
  stack-frame layout, value-flow, or bounded runtime probes.
- The repair addresses a semantic backend cause for wrong value materialization
  or frame addressing, not a named-testcase shortcut.
- The focused proof preserves the repaired timeout behavior: `00205` must not
  return to the 5-second timeout path, and generated loop-header compares must
  remain conditional compares rather than an unconditional loop.
- The focused proof preserves the idea 303 legality result: generated assembly
  must not return to illegal `sxtw` with a 32-bit destination register.
- No expectation, allowlist, unsupported classification, timeout policy,
  runner behavior, proof-log policy, or CTest registration change is used as
  capability progress.

## Reviewer Reject Signals

Reject the route if it:

- special-cases filename `00205`, specific source line numbers, specific case
  indexes, temporary labels, or exact stack offsets such as `#632`, `#1064`, or
  `#1496` instead of repairing the semantic frame/value bug;
- changes expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, proof-log policy, or CTest registration while
  claiming backend capability progress;
- claims success because `00205` no longer fails output comparison while nearby
  same-feature aggregate/frame/value-materialization cases remain unsupported
  or unexamined when the diff touched shared lowering code;
- regresses the idea 303 sign-extension legality result or the idea 304
  loop-bound compare timeout repair;
- hides the same out-of-frame read pattern behind helper renames,
  classification-only changes, or narrower proof selection;
- performs a broad ABI, stack-frame, aggregate, or instruction-selection
  rewrite before tying the `00205` mismatch to a concrete generated-code
  mechanism.
