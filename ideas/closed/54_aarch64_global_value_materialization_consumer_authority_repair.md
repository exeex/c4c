# AArch64 Global Value Materialization Consumer Authority Repair

## Goal

Repair remaining AArch64 global-load value materialization consumers outside
`dispatch_value_materialization.cpp` so they consume shared prepared global
address policy instead of recovering global symbols or GOT/direct policy
locally.

## Why This Exists

The Step 5 route review for
`ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md`
found legitimate adjacent duplicate-authority candidates in
`globals.cpp` and `fp_value_materialization.cpp`. They should not be executed
under the dispatch-value-materialization source idea because that idea owns
only `dispatch_value_materialization.cpp`.

## In Scope

- Audit `src/backend/mir/aarch64/codegen/globals.cpp` global-load
  materialization helpers, especially
  `make_load_global_got_materialization_instruction`, for local global-symbol
  or GOT/direct policy recovery that should consume shared prepared authority.
- Audit `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` FP
  producer `LoadGlobal` materialization for the same local global-symbol or
  GOT/direct policy recovery.
- Reuse or extend the shared prepared global address policy surface added for
  dispatch value materialization when that surface is sufficient.
- Add a narrow shared query only if the existing prepared address policy cannot
  express a needed non-dispatch consumer contract.

## Out Of Scope

- Do not edit `dispatch_value_materialization.cpp` as part of this follow-up
  except for mechanical compatibility with a shared query that these consumers
  require.
- Do not change AArch64 instruction spelling, relocation mechanics, or
  GOT/direct emission behavior except to consume a shared semantic contract.
- Do not fold unrelated memory, ALU, calls, comparison, publication, or
  dispatch select-chain repairs into this idea.
- Do not weaken supported-path expectations or mark existing supported global
  materialization routes unsupported.

## Acceptance Criteria

- Remaining non-dispatch global-load value materialization consumers no longer
  recover global symbol identity or GOT/direct policy through local name
  spelling when shared prepared authority is available.
- Any new shared query has a narrow consumer contract and is exercised by the
  global-load and FP materialization routes it is meant to serve.
- Validation covers both the ordinary global-load materialization route and the
  FP `LoadGlobal` materialization route, with enough broader backend proof to
  catch top-level global-load lowering regressions.

## Reviewer Reject Signals

- Reject named-case shortcuts around
  `make_load_global_got_materialization_instruction` or FP `LoadGlobal`
  producers whose main evidence is one testcase.
- Reject hard-coded global-name spelling, `@`-prefix matching, module-global
  searches, or local GOT/direct policy inference claimed as the final repair
  when shared prepared address policy can carry the fact.
- Reject expectation downgrades, unsupported-test rewrites, helper renames, or
  classification-only changes claimed as global materialization capability
  progress.
- Reject broad rewrites to unrelated AArch64 memory, ALU, calls, comparison, or
  dispatch value-materialization routes under this idea.
- Reject retaining the old local global-symbol or GOT/direct policy recovery
  behind a newly named helper without changing the authority source.

## Closure Note

Closed after Step 5 final validation. Ordinary global-load materialization and
FP `LoadGlobal` materialization now consume the shared prepared global-address
authority, with the narrow shared query exercised by the focused global/FP
materialization scope.

Canonical close logs used the same focused 5-test command before and after;
both passed 5/5. The close-time regression guard with
`--allow-non-decreasing-passed` passed with no new failures. Broader backend
validation remained 165/167 with only the known failures
`backend_aarch64_instruction_dispatch` and
`backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`.
