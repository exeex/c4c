# AArch64 Variadic Frame And Formal Publication

Status: Closed
Created: 2026-05-19
Split From: ideas/closed/323_aarch64_vararg_consumption_source_progression.md
Closed: 2026-05-19

## Goal

Repair AArch64 variadic function frame-slot publication and formal argument
preservation so generated functions allocate enough stack space for all
referenced local slots and preserve incoming formals before variadic setup or
loop code reuses argument registers.

## Why This Exists

Idea 323 repaired the aggregate/floating `va_arg` source/progression path.
The focused `00204.c` representative no longer contains raw
`va.arg.aggregate*` helper text, still materializes the `va_start` destination
before field stores, and now uses FP register-save-area source selection with
`FpOffset` progression for HFA/floating aggregate consumers.

The remaining first bad fact is earlier frame/formal correctness in generated
`myprintf`: the function allocates only `896` bytes but later references local
or spill slots far beyond that frame, such as `[sp, #9696]`, and it clobbers
the incoming format pointer with `mov x0, x21` before the format loop reads
from `x0`. The owner is AArch64 local/frame-slot publication and formal
argument preservation in variadic functions, not aggregate `va_arg`
source/progression.

## In Scope

- Localize why generated AArch64 variadic functions can emit stack-slot
  accesses beyond the allocated frame size.
- Localize why the incoming fixed formal, especially the `format` pointer in
  `myprintf`, is overwritten or not published to the expected local home before
  variadic prologue or loop code consumes it.
- Repair general frame-slot publication, frame-size accounting, local home
  materialization, or formal preservation for AArch64 variadic functions.
- Add focused backend coverage that proves frame allocation covers emitted
  stack slots and fixed formals survive variadic setup.
- Preserve repairs from ideas 314, 315, 317, 318, 319, 320, 321, 322, and
  323.
- Use `c_testsuite_aarch64_backend_src_00204_c` as the representative external
  proof after local backend coverage is repaired.

## Out Of Scope

- Reopening aggregate/floating `va_arg` source selection or `FpOffset`
  progression unless generated evidence proves those paths still own the first
  bad fact.
- Reopening `va_start` destination address publication, raw aggregate helper
  text lowering, F128 transport addressability, HFA argument call-lane
  lowering, scalar ALU immediate materialization, large frame adjustment
  materialization, or large stack-offset spelling without direct generated-code
  evidence.
- Fixing unrelated global initializer emission, unrelated runtime output
  mismatches, runners, timeout policy, expectations, unsupported
  classifications, proof-log policy, or CTest registration.
- Hiding oversized offsets by changing tests, forcing one known local into a
  special slot, or adjusting only the `00204.c` generated shape.

## Acceptance Criteria

- The out-of-frame stack-slot references and fixed-formal clobber are
  localized to concrete AArch64 frame, prepared-home, local-publication,
  formal-preservation, or printer surfaces.
- The repair is a general AArch64 frame/local/formal publication rule for
  variadic functions, not a `00204.c`, `myprintf`, `format`, `x0`, `x21`, one
  local variable, one stack slot, one offset, or one emitted-line shortcut.
- Focused backend coverage proves emitted stack references are covered by the
  allocated frame and incoming fixed formals remain available to generated
  user code after variadic setup.
- Prior-owner guardrails remain cleared under a fresh build and focused CTest
  proof.
- If `00204.c` advances to another first bad fact outside frame/local/formal
  publication, the residual is classified into a separate owner.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `stdarg`, `myprintf`, `format`, `x0`, `x21`, one
  local variable, one stack slot, one offset, or one emitted instruction
  sequence instead of repairing general AArch64 variadic frame/formal
  publication;
- hides the fault by weakening expectations, unsupported classifications,
  runner behavior, timeout policy, proof-log policy, or CTest registration;
- claims progress while generated code can still reference stack slots outside
  the allocated frame or consume a clobbered fixed formal before publication;
- reopens aggregate `va_arg` source/progression, `va_start` destination
  publication, raw helper lowering, F128 transport, HFA argument ABI, scalar
  immediates, large stack/frame materialization, or global initializer
  emission without generated-code evidence tying that work to the current
  frame/formal fault;
- adds only external c-testsuite coverage without focused backend assertions
  for frame-size coverage and fixed-formal preservation.

## Closure Note

Idea 324 is complete. The implemented frame/formal publication repair makes
generated `myprintf` allocate a frame covering its emitted homes, removes the
prior `[sp, #9696]` out-of-frame family, publishes `%p.format` with
`str x0, [sp, #624]`, removes the bad entry `mov x0, x21`, and preserves
`va_start` destination materialization before field stores.

The remaining `00204.c` runtime segfault is outside this idea's scope. Fresh
generated-output evidence points to ordinary local/value-home initialization
and constant/pattern operand publication in variadic functions: `myprintf`
reads homes such as `[sp, #640]`, `[sp, #648]`, and `[sp, #656]`, and compares
`w13`, before same-function publication of those values. That residual is
tracked by `ideas/open/325_aarch64_variadic_local_value_home_publication.md`.
