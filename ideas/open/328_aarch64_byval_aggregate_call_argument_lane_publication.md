# AArch64 Byval Aggregate Call Argument Register-Lane Publication

Status: Open
Created: 2026-05-19
Split From: ideas/open/327_aarch64_fixed_formal_entry_publication.md

## Goal

Repair AArch64 caller-side publication for small byval aggregate call
arguments, so prepared object or frame-slot bytes are packed into the AAPCS64
integer argument register lanes before the call.

## Why This Exists

Idea 327 repaired the callee-side fixed-formal entry publication gap. The
committed proof for `de571342a` moved the `00204.c` representative's first bad
fact to the caller side in function `arg`: before `bl fa_s1`, generated
assembly copies the local `s1` aggregate into a prepared stack slot such as
`[sp, #928]` but never publishes the byte into `x0`; before `bl fa_s2`, the
callsite likewise prepares local stack bytes but fails to pack the two-byte
aggregate into `x0`.

The callee now expects the ABI register lanes and publishes them correctly on
entry. The remaining capability gap is call-argument lowering: small byval
aggregates that AAPCS64 passes through integer registers must be loaded from
their prepared aggregate storage, packed into the correct `xN` lane bytes, and
available in the ABI argument registers at the branch.

## In Scope

- Localize AArch64 call-argument lowering for byval aggregate arguments whose
  ABI classification passes their bytes in integer registers.
- Map each byval aggregate argument from semantic/prepared source object bytes
  or frame slots into the ABI argument register lanes required by AAPCS64.
- Repair register-lane publication for small byval aggregates, including
  byte packing across one or more integer argument registers for sizes 1
  through 16 when classified for register transport.
- Preserve stack-passed handling for larger byval aggregates such as the
  `s17` family when the ABI requires stack transport.
- Add focused backend coverage that fails without caller-side byval register
  lane publication and passes with the repair.
- Rerun the focused `00204.c` representative proof to determine whether the
  next first bad fact returns to the HFA/floating residual path from idea 326
  or exposes another distinct owner.

## Out Of Scope

- Reopening fixed-formal entry publication from idea 327 unless fresh
  generated-code evidence again shows a callee consuming an unpublished fixed
  formal before first use.
- Reopening HFA/floating `va_arg`, floating register-save-area progression,
  overflow-area progression, or HFA lane materialization unless the
  representative advances to those owners after byval call-argument
  publication is repaired.
- Reopening global initializer emission, fixed HFA argument lanes, fixed HFA
  return lanes, local/value-home publication, frame/formal publication,
  `va_start` destination publication, aggregate helper text lowering, F128
  transport, scalar ALU immediate materialization, large frame adjustment, or
  stack-slot spelling without direct generated-code evidence.
- Changing semantic admission, runners, timeout policy, expectations,
  unsupported classifications, proof-log policy, or CTest registration.
- Fixing only `00204.c`, `arg`, `fa_s1`, `fa_s2`, `s1`, `s2`, one stack
  offset, one byte count, one source slot, one argument register, or one
  emitted instruction sequence.

## Acceptance Criteria

- The caller-side byval aggregate call-argument gap is localized to concrete
  semantic arguments, prepared storage records, ABI classifications, source
  bytes, destination register lanes, and emitted callsite code.
- The repair publishes small byval aggregate call arguments generally from
  prepared aggregate storage into the AAPCS64 integer argument register lanes
  before the call.
- Focused backend coverage proves at least one single-register and one
  multi-lane or multi-byte byval aggregate call argument is packed into ABI
  register lanes from prepared storage before `bl`, while larger stack-passed
  byval aggregates, including size 17 and larger shapes, remain stack-passed.
- Adjacent guardrails from ideas 324 through 327 remain stable, including
  frame/formal publication, local/value-home publication, fixed HFA
  argument/return lanes, HFA global initializers, and fixed-formal entry
  publication.
- The focused `00204.c` representative either passes or advances to a newly
  classified first bad fact for lifecycle handoff.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00204.c`, `arg`, `fa_s1`, `fa_s2`, `s1`, `s2`, `s17`, one
  stack offset such as `[sp, #928]`, one aggregate size, one ABI register such
  as `x0`, or one emitted callsite sequence instead of repairing general byval
  aggregate call-argument register-lane publication;
- weakens expectations, unsupported classifications, runner behavior, timeout
  policy, proof-log policy, or CTest registration to hide the remaining
  representative failure;
- claims capability progress only through helper renames, dump text changes,
  expectation rewrites, or classification notes without generated callsites
  loading prepared aggregate bytes and packing them into ABI argument
  registers before `bl`;
- rewrites callee fixed-formal entry publication, HFA/floating variadic
  lowering, global data, frame layout, local/value-home publication, or large
  stack behavior without fresh generated-code evidence tying that owner to the
  current first bad fact;
- treats larger stack-passed byval aggregates as register-passed, or treats
  register-passed small byval aggregates as stack-only, contrary to the
  observed AAPCS64 classification;
- leaves the exact old failure mode in place behind a new abstraction, such
  that calls to `fa_s1` or `fa_s2` still branch without publishing the
  prepared aggregate bytes into the ABI register lanes the callee now reads.

## Lifecycle Handoff

2026-05-20: Step 4 validated the idea 328 capability and classified the
remaining `00204.c` failure as a distinct adjacent owner. Generated AArch64
for caller `arg` now publishes the prepared `s1` byte into `w0` before
`bl fa_s1` and packs the prepared `s2` bytes into `x0` before `bl fa_s2`.
The remaining crash is inside `myprintf`: after the `%9s` aggregate
`va_arg` path, generated code reaches libc `printf` with `x0 == 0x1` instead
of loading `.str33` (`"%.9s"`) into `x0`; the `%7s` path has the same
missing `.str31` shape. That residual is not remaining caller-side byval
aggregate lane publication and is not yet the parked HFA/floating residual.

Close was not accepted in this lifecycle pass because the existing
`test_before.log`/`test_after.log` regression-guard comparison for the
12-test focused scope had no new failures but did not strictly increase the
pass count. Keep this idea open and parked unless the supervisor accepts a
non-decreasing close guard or provides closure-grade logs. The adjacent
variadic aggregate post-`va_arg` call setup work is tracked separately in
`ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`.

2026-05-20: Reactivated after idea 326's repaired HFA/floating argument route
advanced the representative to another fixed non-HFA byval aggregate call
boundary fault. The current first bad fact is `fa1(struct s8, struct s9,
struct s10, struct s11, struct s12, struct s13)`: expected
`stu ABC JKL TUV 456 ghi`, observed `stu ABC I JKL RS TUV`.

Generated BIR for caller `arg` materializes source bytes for `%t103` through
`%t108` from globals `s8` through `s13` correctly, then calls
`fa1(ptr byval(size=8) %t103, ptr byval(size=9) %t104, ptr byval(size=10)
%t105, ptr byval(size=11) %t106, ptr byval(size=12) %t107, ptr
byval(size=13) %t108)`. The generated caller publishes contiguous byval
chunks through `x0` through `x6`: `x0` carries `s8`, `x1` carries the first
eight bytes of `s9`, `x2` carries the trailing `I`, `x3` carries `s10`, `x4`
carries trailing `RS`, `x5` carries `s11`, and `x6` carries trailing `YZ0`.
Generated `fa1` entry then treats `x0` through `x5` as the starts of six
separate byval formals, explaining the observed shifted string fragments.

Resume this idea from AAPCS64 fixed non-HFA byval aggregate argument
placement, including per-aggregate rounded register-slot consumption and the
coherent register-to-stack transition when too few GPR argument registers
remain for a whole aggregate. Preserve the earlier single-aggregate
register-lane publication repairs; do not collapse the residual into a named
`00204.c` or `fa1` workaround.
