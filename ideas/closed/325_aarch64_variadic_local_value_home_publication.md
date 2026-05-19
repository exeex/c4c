# AArch64 Variadic Local Value Home Publication

Status: Closed
Created: 2026-05-19
Closed: 2026-05-19
Split From: ideas/closed/324_aarch64_variadic_frame_formal_publication.md

## Goal

Repair AArch64 variadic function local/value-home initialization and
constant/pattern operand publication so generated control flow, matcher logic,
and printf operand paths only read stack/register homes after those homes have
been defined in the current function.

## Why This Exists

Idea 324 repaired the frame/formal publication blocker: generated `myprintf`
now allocates a frame covering emitted stack homes, publishes `%p.format`
before variadic setup, avoids the bad entry `mov x0, x21`, and keeps
`va_start` destination address materialization intact. Raw aggregate helper
text remains absent, and aggregate/floating `va_arg` source/progression still
uses executable register-save-area and overflow paths.

The next first bad fact in the focused `00204.c` representative is ordinary
local/value-home consumption before publication. Generated `myprintf` reads
matcher and printf operand homes such as `[sp, #640]`, `[sp, #648]`,
`[sp, #656]`, and related slots without same-function stores before first
use, and emits an immediate `cmp w13, #0` with no preceding local definition
for `w13`.

## In Scope

- Localize which prepared local homes, constants, pattern operands, matcher
  temporaries, or printf operands are consumed before publication in AArch64
  variadic functions.
- Repair the general publication path so local/value homes are initialized
  before generated control-flow, matcher, and operand-consumer code reads
  them.
- Preserve AArch64 frame-size coverage, fixed-formal publication, `va_start`
  destination materialization, aggregate helper lowering, F128 transport,
  aggregate/floating `va_arg` source/progression, HFA argument lanes, scalar
  ALU immediates, and large stack/frame materialization.
- Add focused backend coverage for local/value-home publication before use.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative
  external proof after local backend coverage is repaired.

## Out Of Scope

- Reopening idea 324's frame-size coverage or fixed-formal publication unless
  generated output again shows uncovered stack references or clobbered fixed
  formals.
- Reopening `va_start` destination address publication, aggregate helper text
  lowering, F128 transport addressability, HFA argument call-lane lowering,
  aggregate/floating `va_arg` source/progression, scalar ALU immediate
  materialization, or large stack/frame materialization without direct
  generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only one named `00204.c`, `myprintf`, format-loop, stack offset,
  local slot, register, or instruction sequence.

## Acceptance Criteria

- The read-before-publication local/value-home fault is localized to concrete
  prepared-home, local-publication, constant/pattern operand, dispatch, or
  printer surfaces.
- The repair is a general AArch64 local/value-home publication rule for
  variadic generated code, not a named-test or named-offset shortcut.
- Focused backend coverage proves relevant stack/register homes are published
  before control-flow tests, matcher paths, or printf operand consumers read
  them.
- Prior-owner guardrails remain cleared under a fresh build and focused CTest
  proof.
- If `00204.c` advances to another first bad fact outside local/value-home
  publication, the residual is classified into a separate owner.

## Completion Notes

The Step 2 and Step 3 publication repairs were committed as
`5001cecf6 repair AArch64 local value publication`. `todo.md` recorded
focused local coverage for branch/control values, symbol-address call
provenance, frame-slot address call operands, mutable pointer-local
provenance, and predecessor/join source publication.

The focused representative still fails, but the first remaining bad fact is
not an ordinary local/value-home publication fault: `fa_hfa11(hfa11)` prints
`0.0` instead of `11.1`, followed by corrupted floating/HFA output and a later
segmentation fault. The review artifact
`review/325_step2_route_review.md` judged the implementation aligned with this
idea and recommended lifecycle classification for the HFA/floating residual
unless fresh generated-code evidence ties it back to local/value-home
publication.

Closure classification: idea 325 is complete. The remaining failure is tracked
as `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`.

Close-time guard: existing focused before/after logs cover the Step 2/3 proof
scope. Strict-increase mode reports no additional resolved CTest case because
the representative remains the only failing CTest test, but
`--allow-non-decreasing-passed` reports no new failing tests and no pass-count
decrease for the matching scope.

## Reviewer Reject Signals

Reject any claimed continuation of this closed idea if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, the format loop, one stack
  slot, one register, one local variable, one constant, or one emitted
  instruction sequence instead of repairing general AArch64 local/value-home
  publication;
- hides the fault by weakening expectations, unsupported classifications,
  runner behavior, timeout policy, proof-log policy, or CTest registration;
- claims progress while generated code still reads ordinary local/value homes,
  constants, or pattern operands before same-function publication;
- reopens frame/formal publication, aggregate `va_arg`, `va_start`, raw helper
  lowering, F128 transport, HFA argument ABI, scalar immediate, or large
  frame/stack owners without generated-code evidence tying that work to a
  local/value-home publication fault;
- adds only external c-testsuite coverage without focused backend assertions
  for publication before use.
