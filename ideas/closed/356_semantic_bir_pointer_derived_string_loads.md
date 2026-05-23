# Semantic BIR Pointer-Derived String Loads

Status: Closed
Created: 2026-05-21
Split From: ideas/open/355_aarch64_address_valued_memory_call_argument_publication.md

## Closure

Closed: 2026-05-23

The focused semantic/backend pointer-derived byte-load owner is satisfied. The
source note already recorded that the old owned first bad fact was absent:
dynamic pointer-derived byte loads no longer collapse to fixed `@.str0` global
byte loads, and the remaining string-literal pointer publication residual was
split to `ideas/open/365_aarch64_string_literal_pointer_value_publication.md`.

Close was accepted under the supervisor-approved non-decreasing close policy.
Matching fresh focused proof logs in `test_before.log` and `test_after.log`
cover:
`backend_lir_to_bir_notes`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_prepared_memory_operand_records`, and
`c_testsuite_aarch64_backend_src_00173_c`. The regression guard reported
before 5/5, after 5/5, delta 0, and no new failures with
`--allow-non-decreasing-passed`.

## Parked Outcome

Parked: 2026-05-21

Refresh: 2026-05-22

Reactivated for a fresh first-bad-fact check after several adjacent parked
owners were refreshed. The focused semantic/backend pointer-derived byte-load
subset stayed green:
`backend_lir_to_bir_notes`, `backend_aarch64_instruction_dispatch`,
`backend_aarch64_memory_operand_contract`,
`backend_aarch64_prepared_memory_operand_records`, and
`c_testsuite_aarch64_backend_src_00173_c` all passed. No current `00173` or
pointer-derived byte-load first bad fact remains under this owner.

Close was rejected again because the required monotonic regression guard on
matching `test_before.log` / `test_after.log` reported
`passed=5 failed=0 total=5` before and after, so the pass count did not
strictly increase. Keep this idea parked and reactivate only if fresh evidence
shows dynamic pointer-derived byte loads collapsing back to fixed global-byte
loads.

The semantic dynamic pointer-derived byte-load owner was repaired by commit
`96b80ee21 Preserve pointer-carrier byte loads`. Focused backend proof after
the repair kept `backend_lir_to_bir_notes`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_memory_operand_contract` passing, and `00173` advanced beyond
the old fixed-global-byte load behavior.

The repaired semantic BIR no longer collapses dynamic `*b` / `*src` consumers
to fixed `LoadGlobalInst @.str0` byte loads. Those consumers now remain dynamic
pointer-value loads, and prepared addressing records them as
`base=pointer_value`.

The remaining `00173` failure is a downstream string literal pointer-address
publication residual, not this idea's dynamic byte-load owner. The source
assignment `char *a = "hello"` stores pointer carrier `%t2` into `%lv.a`, but
`%t2` is never materialized as the `.str0` address. Prepared BIR treats `%t2`
as a frame spill (`slot#26+stack72`), and generated AArch64 stores `sp+72` as
the runtime pointer. Later `printf`, `*a`, `*b`, and `*src` consumers correctly
follow that dynamic pointer, but the pointer value itself is wrong. The split
follow-up is
`ideas/open/365_aarch64_string_literal_pointer_value_publication.md`.

Close was rejected because the hook full-suite baseline candidate after
`96b80ee21` was not monotonic despite the focused semantic/backend progress.
This idea is therefore parked rather than moved to `ideas/closed/`.

## Goal

Preserve dynamic pointer-derived byte loads in semantic BIR instead of
collapsing them to fixed global string-byte loads.

## Why This Exists

During idea 355 Step 4 classification, `00173` remained a timeout after the
AArch64 address/pointer/pointee publication repairs. Read-only BIR evidence
showed the first bad fact before AArch64 lowering: loop expressions such as
`*b` and `*src` are already represented as fixed
`bir.load_global i8 @.str0` / `@.str0+1` accesses. The generated loop then
tests the first string byte repeatedly instead of reading through the current
incremented pointer value.

This is a semantic-BIR value modeling issue, not an AArch64 publication issue.

## In Scope

- Localize where semantic lowering loses the dynamic pointer base for
  pointer-derived string or array byte loads.
- Repair BIR construction so loads through an incremented pointer use that
  pointer value as the memory base.
- Add focused semantic or backend-route coverage that proves dynamic pointer
  loads remain dynamic before AArch64 codegen.
- Prove `c_testsuite_aarch64_backend_src_00173_c` advances or passes if the
  semantic repair reaches generated code.

## Out Of Scope

- AArch64 address-valued memory or call-argument publication already covered by
  idea 355.
- Timeout policy, runner behavior, CTest registration, proof-log handling, or
  expectation changes.
- Filename-specific handling for `00173`, one source string literal, one loop
  shape, or one generated instruction sequence.
- Same-module recursive pointer-formal/callee-saved-home publication for
  `00181`.
- Frontend admission work for `00005`.

## Acceptance Criteria

- The first bad fact is localized to a semantic-BIR lowering boundary that
  turns a pointer-derived load into a fixed global-byte load.
- Focused coverage fails before the repair and passes after it, proving the
  dynamic pointer base is preserved.
- `00173` advances or passes without expectation, timeout, runner, or
  filename-specific changes.
- Nearby string or pointer byte-load cases are checked enough to show the fix
  is semantic rather than testcase-shaped.

## Reviewer Reject Signals

Reject the route if it:

- special-cases `00173`, one string literal, one global symbol, one loop body,
  or one emitted AArch64 instruction pattern;
- claims progress by changing timeout policy, runner behavior, unsupported
  classifications, proof logs, or external test expectations;
- moves the failure later by renaming helpers or reshaping dumps while semantic
  BIR still contains fixed `@.str0` byte loads for pointer-derived accesses;
- broadens into AArch64 call publication, recursive formal homes, ABI work, or
  unrelated frontend admission without a lifecycle split;
- leaves dynamic `*p`/`*src`-style byte loads unable to use the current pointer
  value as the memory base.
