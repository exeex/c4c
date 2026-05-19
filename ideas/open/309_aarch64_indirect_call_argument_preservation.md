# AArch64 Indirect Call Argument Preservation

Status: Open
Created: 2026-05-19

## Goal

Repair AArch64 call lowering so an indirect function-pointer callee and its
arguments are preserved across nested call setup, with `00189.c` as the focused
runtime proof surface.

## Why This Exists

After focused idea 308 repaired the old externally binding `stdout` PIC
relocation failure, `c_testsuite_aarch64_backend_src_00189_c` now reaches
runtime and fails with `RUNTIME_NONZERO exit=Segmentation fault`.

Umbrella idea 295 Step 2 compared `00189.c` against nearby runtime
nonzero/crash/mismatch residuals and found no broader shared semantic owner
ready to split. The current generated AArch64 for `00189.c` points at a
specific call-boundary failure: `main` loads `stdout` and a function pointer,
then clobbers or fails to preserve the indirect callee and argument values
before `blr x21`. The nested `(*f)(24)` result is also not preserved as the
later integer vararg.

This is intentionally a singleton focused owner. It is acceptable only because
the scope is a semantic call-lowering capability, not a filename-specific
shortcut.

## In Scope

- Inspect and repair AArch64 lowering for indirect function-pointer calls where
  callee and argument values must survive nested call setup.
- Preserve the indirect callee register/value through argument materialization
  until the final `blr`.
- Preserve argument values for the outer indirect call when one argument is
  produced by a nested direct or indirect call.
- Cover the focused c-testsuite target
  `c_testsuite_aarch64_backend_src_00189_c`.
- Add or update narrow backend tests only when they assert semantic
  indirect-call callee and argument preservation behavior, not the exact
  c-testsuite filename.

## Out of Scope

- Direct multi-argument call shuffle bugs parked around `00181.c` and
  `00182.c`.
- Direct-call vararg aliasing or vararg materialization bugs parked around
  `00200.c`.
- Address-of-local direct-call argument preparation parked around `00218.c`.
- Dynamic-stack/goto local value materialization, string/pointer/store/control
  materialization, timeout, output-storm, and unrelated runtime mismatch
  buckets from umbrella idea 295.
- Reopening closed owners 285 through 308 without generated-code or proof
  evidence that contradicts their closure boundary.
- Expectation, allowlist, unsupported-classification, CTest registration,
  runner, timeout-policy, proof-log, or test-contract changes to improve
  counts.

## Acceptance Criteria

- `c_testsuite_aarch64_backend_src_00189_c` no longer segfaults from clobbered
  indirect callee or outer-call argument setup.
- Generated AArch64 for `00189.c` preserves the function-pointer callee until
  the `blr` and places the required outer-call arguments in the correct call
  registers after nested call setup.
- Focused backend tests, if added or updated, cover indirect callee
  preservation and nested-call result preservation as semantic behavior.
- A fresh build or compile proof is recorded for the implementation slice.
- Supervisor-selected proof includes the focused c-testsuite target and enough
  adjacent call-boundary coverage to reject a named-test-only fix.

## Deactivation Note

Paused 2026-05-19 after Step 2 found that the outer indirect call's string
argument identity is already lost before AArch64 lowering. LIR still has
`%t2 = getelementptr ... @.str1`, but BIR/prepared data publish the outer call
argument only as an ordinary register source and do not emit a prepared
string-constant address materialization for `.str1`. An AArch64-only repair
would have to guess from source shape, argument index, or assembly text, which
would violate this idea's reject signals.

Lifecycle switched to
`ideas/open/310_prepared_indirect_call_string_argument_facts.md` as a
prerequisite producer-fact owner. When that idea is complete, return here to
finish the AArch64 indirect-call callee/register preservation and re-check the
observed `fprintfptr` callee register placement mismatch before proving
`c_testsuite_aarch64_backend_src_00189_c`.

## Reactivation Note

Reactivated 2026-05-19 after idea 310 repaired and proved the prerequisite
producer fact. The focused prepared dump for `00189.c` now publishes
`address_materialization block=entry inst_index=4 kind=string_constant
result=@.str1 text=.str1 ...`, so the remaining route is back inside AArch64
indirect-call callee/register preservation.

The next packet should inspect the remaining `fprintfptr` callee register
placement mismatch before choosing the repair surface. The previously observed
hazard was that prepared storage printed `%t0` as `reg=x21`, while ordinary
non-GOT `load_global` lowering loaded `fprintfptr` as `x20`.

## Reviewer Reject Signals

Reject the route if it:

- fixes only `00189.c` by filename, source-shape matching, hard-coded `stdout`,
  hard-coded `fprintfptr`, or emitted-instruction matching instead of repairing
  indirect-call callee and argument preservation semantically;
- claims progress through expectation, allowlist, unsupported-classification,
  CTest registration, runner behavior, timeout policy, proof-log, or
  test-contract changes;
- broadens the implementation into direct multi-argument shuffle, direct
  vararg aliasing, address-of-local direct-call argument preparation, timeout,
  output-storm, or unrelated runtime buckets parked under idea 295;
- reopens closed owners 285 through 308 without generated-code evidence that
  contradicts their closure boundaries;
- claims capability progress from helper renames, classification-only edits, or
  diagnostic text changes while the generated indirect call still clobbers the
  callee or outer-call arguments before `blr`;
- hides the original failure behind a generic call-lowering abstraction while
  retaining the same bad value-preservation behavior.
