# AArch64 Fixed Formal Entry Publication

Status: Closed
Created: 2026-05-19
Closed: 2026-05-22
Split From: ideas/open/326_aarch64_variadic_hfa_floating_residual.md

## Goal

Repair AArch64 function-entry publication for fixed formals whose prepared
home is not the incoming AAPCS64 argument register, so every fixed parameter is
available from its assigned storage before first use.

## Why This Exists

Idea 326 advanced `00204.c` past the prior HFA global initializer, fixed HFA
argument, and fixed HFA return blockers. Step 4 then classified the current
runtime segmentation fault as a distinct adjacent issue: generated code enters
`myprintf("%9s ...", ...)` with the fixed pointer formal in incoming `x0`, but
prepared AArch64 storage assigns `%p.format` to `register:x13` and the callee
first consumes that unpublished value.

The semantic BIR handoff is correct (`bir.func @myprintf(ptr %p.format)` and
`bir.store_local %lv.s, ptr %p.format`). The missing capability is AArch64
entry publication from the ABI incoming location into the prepared parameter
home when those locations differ.

## In Scope

- Localize how AArch64 prepared storage assigns fixed function parameters to
  homes such as registers or stack slots.
- Repair function-entry parameter publication from AAPCS64 incoming argument
  registers into prepared homes before first use.
- Cover ordinary fixed formals in variadic callees, including the first pointer
  formal represented by `myprintf(ptr %format, ...)`.
- Add focused prepared/codegen tests proving that fixed formals assigned to
  non-ABI homes are populated from incoming ABI registers.
- Rerun the focused `00204.c` representative proof to determine whether the
  next first bad fact returns to the HFA/floating residual path.

## Out Of Scope

- Repairing HFA/floating `va_arg` source selection, register-save-area
  progression, overflow-area progression, or HFA lane materialization unless
  fresh post-publication evidence reaches those paths.
- Reopening global initializer emission, fixed HFA argument lanes, fixed HFA
  return lanes, local/value-home publication, `va_start` destination
  publication, aggregate helper text lowering, F128 transport, scalar ALU
  immediate materialization, large frame adjustment, or stack-slot spelling
  without direct generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only `00204.c`, `myprintf`, `%p.format`, `x0`, `x13`, one stack slot,
  one register, one pointer type, or one emitted instruction sequence.

## Acceptance Criteria

- The fixed-formal entry publication gap is localized to concrete prepared
  records, ABI incoming locations, assigned homes, and emitted prologue or
  first-use code.
- The repair publishes fixed parameters generally when their assigned prepared
  homes differ from the ABI incoming argument locations.
- Focused backend coverage proves at least one ordinary fixed formal in a
  variadic callee is published before first use, and adjacent existing AArch64
  publication guardrails remain stable.
- The focused `00204.c` representative either gets past the current
  `%p.format` null dereference or advances to a newly classified first bad fact
  for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `myprintf`, `%p.format`, `.str49`, `x0`, `x13`, one
  register assignment, one pointer type, one local cursor, or one emitted
  instruction sequence instead of repairing general fixed-formal entry
  publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the failure;
- claims progress only by rewriting tests, renaming helpers, or classifying
  the segfault without adding publication from ABI incoming locations to
  prepared parameter homes;
- broadens into HFA/floating `va_arg`, global initializer, fixed HFA
  argument/return, local/value-home, or frame-layout rewrites without fresh
  generated-code evidence tying those owners to the current first bad fact;
- leaves the exact old failure mode in place behind a new abstraction, such
  that generated `myprintf` can still consume an unpublished fixed pointer
  formal before first use.

## Lifecycle Handoff

2026-05-19: Committed slice `de571342a` repaired the fixed-formal entry
publication owner for ordinary register homes and small byval aggregate
formals prepared into frame slots. Focused backend coverage now proves the
callee-side publication behavior, and generated `00204.c` shows `myprintf`
publishing the incoming `x0` fixed pointer into its prepared home before use.
The same proof moved the representative's first bad fact to caller-side byval
aggregate call-argument publication: before calls such as `fa_s1` and
`fa_s2`, generated AArch64 prepares source aggregate bytes in stack/frame
storage but does not pack those bytes into the AAPCS64 integer argument
register lanes expected by the callee.

This idea is parked rather than kept active. Do not continue the caller-side
byval handoff repair as routine fixed-formal entry publication work. Resume or
close this idea only after lifecycle authority is allowed to touch the closed
archive, or if fresh generated-code evidence shows the exact fixed-formal
entry publication fault has returned.

2026-05-22: Lifecycle close accepted after the refreshed Step 1 proof found no
live in-scope fixed-formal entry publication owner. The focused proof covered
`backend_aarch64_instruction_dispatch`, `backend_aarch64_call_boundary_owner`,
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`,
and `c_testsuite_aarch64_backend_src_00204_c`; all passed. The close gate used
the existing canonical focused proof log read-only because the delegated
operation forbade touching test logs.
