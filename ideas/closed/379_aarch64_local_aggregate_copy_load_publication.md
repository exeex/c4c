# AArch64 Local Aggregate Copy Load Publication

Status: Closed
Created: 2026-05-21
Split From: ideas/open/295_backend_regex_failure_family_inventory.md
Closed: 2026-05-21

## Closure Note

Idea 379 is complete for its local aggregate copy/load publication owner.
Follow-up execution fixed local aggregate address publication, pointer-published
aggregate homes, and fixed aggregate slice-family offsets. The focused
`00216` proof now shows `ls21`, `lu22`, `lv3`, and `flow` matching expected
output, with all 146 backend tests passing in the delegated backend-focused
proof.

The remaining `00216` failure is the previously parked
function-pointer-table/relocation mismatch in `test_multi_relocs`, where the
runtime prints `two/two/two` instead of `one/two/three`. That behavior is
outside this idea's aggregate-copy publication scope and has been split into a
new focused owner under `ideas/open/380_aarch64_function_pointer_table_relocation_dispatch.md`.

## Goal

Repair AArch64 lowering for local aggregate copies and pointer-derived loads so
loads from copied local aggregate state consume a published local address or
value instead of an uninitialized stack slot.

## Why This Exists

The post-378 backend inventory classified the current backend-regex surface as
358 selected tests, 355 passed, and 3 residual failures. Local backend/unit
tests are clean. The only non-timeout residual is
`c_testsuite_aarch64_backend_src_00216_c`; `00200` and `00207` are
timeout-only quarantine cases.

Focused Step 2 evidence reproduces `00216` as a runtime segfault inside
`foo`, before `test_compound_with_relocs` or `test_multi_relocs` can execute.
The debugger stops at `foo+1464` on:

```asm
ldrb w9, [x10]
```

Generated AArch64 loads `x10` through stack slots derived from
`ldr x13, [sp, #496]` with no prior initialization on the observed path:

```asm
ldr x13, [sp, #496]
str x13, [sp, #256]
ldr x9, [sp, #256]
str x9, [sp, #992]
ldr x10, [sp, #992]
ldrb w9, [x10]
```

The relevant source shape in `tests/c/external/c-testsuite/src/00216.c` `foo`
combines local aggregate initializers, local struct copies, pointer loads, and
flexible-array wrapper access, including `struct U lu1 = {3, ls, ...}`,
`struct U lu2 = {3, (ls), ...}`, `const struct S *pls = &ls`,
`struct S ls21 = *pls`, `struct U lu22 = {3, *pls, ...}`,
`struct V lv2 = {(struct S)w->t.s, ...}`, and
`struct V lv3 = {..., ((const struct W *)w)->t.t, ...}`.

Closed idea 374 repaired the earlier stale local aggregate address call
publication shape for `print(ls)`. Current evidence is a later first bad fact:
local aggregate copy/load-local-memory address publication inside `foo`.
The later compound relocation and function-pointer-table portions of `00216`
remain adjacency checks until this crash advances.

## In Scope

- Localize the first `foo` aggregate copy or pointer-derived load whose source
  address/value should be published before the generated `ldrb`.
- Trace the relevant aggregate value through semantic BIR, prepared BIR, MIR,
  and generated AArch64 stack slots.
- Repair the general AArch64 local aggregate copy/load-local-memory
  publication rule so copied local aggregate state and pointer-derived
  subobject loads do not read through uninitialized frame slots.
- Add focused backend coverage for local aggregate copy or wrapper-subobject
  load publication independent of `00216`.
- Prove `c_testsuite_aarch64_backend_src_00216_c` advances past the current
  `foo` segfault or is reclassified by a new first bad fact.

## Out Of Scope

- Reopening closed idea 374 local aggregate address call publication unless
  fresh evidence again shows stale call operands for `&local_aggregate`.
- Compound literal relocation, global relocation, and function-pointer-table
  dispatch work later in `00216` after `foo` advances.
- Timeout-only routes for `00200` shift/type-promotion behavior or `00207`
  dynamic stack/VLA fixed-slot behavior.
- Expectation changes, unsupported classifications, allowlists, runner
  behavior, timeout policy, CTest registration, proof-log policy, or external
  test contract changes.
- Fixing only `00216`, `foo`, `struct S`, `struct U`, `struct V`, `struct W`,
  one local variable, one stack offset, one register, or one emitted
  instruction sequence.

## Acceptance Criteria

- The first bad fact is localized to a concrete local aggregate copy,
  pointer-derived source, prepared value, MIR handoff, or AArch64 frame-slot
  publication boundary.
- Focused backend coverage fails before the repair and passes after it for a
  local aggregate copy/load-local-memory shape that does not depend on the
  external `00216.c` file name.
- `c_testsuite_aarch64_backend_src_00216_c` no longer crashes at the current
  `foo` `ldrb w9, [x10]` through an uninitialized stack-slot-derived pointer.
- If `00216` still fails after the repair, the new failure is classified as a
  later first bad fact, with relocation/function-pointer-table behavior kept
  separate unless evidence proves it shares this owner.
- Recent local aggregate address-call publication guardrails and local
  backend/unit tests selected by the supervisor remain stable.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00216`, `foo`, one aggregate type, one local variable, one
  frame offset such as `[sp, #496]`, one register, or the exact `ldrb`
  neighborhood instead of repairing local aggregate copy/load publication
  generally;
- weakens expectations, unsupported classifications, allowlists, runner
  behavior, timeout policy, proof-log policy, CTest registration, or external
  test contracts;
- claims progress through helper renames, emitted-text reshuffling,
  classification-only notes, or expectation movement while the copied local
  aggregate path can still load through an uninitialized stack slot;
- broadens into compound relocation, global relocation, function-pointer-table
  dispatch, `00200`, `00207`, or closed local-address call publication work
  without fresh first-bad-fact evidence and a lifecycle handoff;
- proves only the external representative while leaving focused backend
  coverage for local aggregate copy/load-local-memory publication absent or
  failing;
- retains the exact old `foo` segfault behind a renamed lowering helper or new
  abstraction boundary.
