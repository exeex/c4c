# Review: idea 358 Step 4 FPR route

Active source idea path: `ideas/open/358_rv64_object_route_abi_width_edges.md`

Chosen base commit: `8fe84dea6 [plan] Activate RV64 ABI width edge coverage plan`

Base selection rationale: this is the lifecycle activation commit for the current
idea 358 runbook, creating `plan.md` and `todo.md` for
`ideas/open/358_rv64_object_route_abi_width_edges.md`. Later lifecycle commits
in this range are `todo_only` execution updates or implementation commits, not a
new source-idea checkpoint.

Commit count since base: 13

Working tree note: `todo.md` is dirty with one extra line, `你該做code review了`,
at line 6. I treated this as transient user/delegation state, not as canonical
execution content.

## Findings

No blocking findings.

Severity: low - one out-of-scope baseline cleanup is present in the active idea
358 diff range.

- `tests/backend/CMakeLists.txt` was changed by `505a86e38 Update RV64
  helper-free variadic CLI test` to convert
  `backend_cli_riscv64_variadic_entry_missing_contract_obj` from an expected
  diagnostic failure into an object-codegen pass. Idea 358 explicitly routes
  variadic/vararg semantics out of this plan and into ideas 360/361
  (`ideas/open/358_rv64_object_route_abi_width_edges.md:32`,
  `ideas/open/358_rv64_object_route_abi_width_edges.md:66`). The associated
  `todo.md` entry labels this as baseline-review cleanup and says not to count
  it as a feature slice, so it is not route drift for the current FPR packet.
  The supervisor should keep treating it as cleanup, not as 358 Step 4 progress.

Severity: info - current `todo.md` contains a transient review prompt line.

- `todo.md:6` contains `你該做code review了`. This is not a route problem, but it
  is not canonical execution metadata. The reviewer role should not edit
  `todo.md`; the supervisor/next executor can remove it when normalizing the
  packet state.

## Alignment Evidence

The current Step 4 route matches the source idea. Idea 358 allows targeted RV64
FPR/simple-call ABI lowering only where prepared BIR already models the needed
semantics (`ideas/open/358_rv64_object_route_abi_width_edges.md:40`,
`ideas/open/358_rv64_object_route_abi_width_edges.md:51`) and rejects guessed
FPR/simple-call behavior without prepared ABI metadata
(`ideas/open/358_rv64_object_route_abi_width_edges.md:104`).

The producer-side identity commit publishes target register identity from ABI
placement instead of forcing object emission to infer register class from
spelling:

- `src/backend/prealloc/target_register_profile.cpp:586` maps RV64 call
  argument/result placements to `PreparedTargetRegisterIdentity` for GPR/FPR
  banks, with an 8-register call slot bound and physical register `10 + slot`.
- `src/backend/prealloc/regalloc/value_homes.cpp:320` attaches that identity to
  fixed formal ABI register homes after the normal ABI register-name selection.
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp:3202`
  verifies a hard-float RV64 formal publishes `bank=Fpr`,
  `register_class=Float`, and `physical_index=10`.

The object admission route consumes the prepared identity and does not parse raw
FPR names:

- `src/backend/mir/riscv/codegen/object_emission.cpp:479` requires
  `PreparedTargetRegisterIdentity` for FPR register numbering and checks RV64,
  FPR bank, Float class, and physical index bounds.
- `src/backend/mir/riscv/codegen/object_emission.cpp:2668` admits formal homes
  only when the existing GPR home helper or the identity-backed FPR helper
  succeeds.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp:3466` proves a
  prepared FPR formal with identity builds, while
  `tests/backend/mir/backend_riscv_object_emission_test.cpp:3490` proves a raw
  `fa0` home without identity is still rejected.

The FPR cast support is narrow and semantic, not testcase-shaped:

- `src/backend/mir/riscv/codegen/object_emission.cpp:2049` lowers only
  `FPExt F32 -> F64` and `FPTrunc F64 -> F32` after source and destination
  homes resolve through the identity-backed FPR helper.
- `src/backend/mir/riscv/codegen/object_emission.cpp:3178` preserves a
  structured diagnostic for other floating casts.
- `tests/backend/mir/backend_riscv_object_emission_test.cpp:1229` builds
  synthetic prepared modules around BIR cast opcode/type pairs and prepared FPR
  homes, not around `src/20030125-1.c` text or source patterns.

## Judgment

Idea-alignment judgment: matches source idea

Runbook-transcription judgment: plan matches idea

Route-alignment judgment: on track

Technical-debt judgment: watch

Validation sufficiency: needs broader proof

Reviewer recommendation: continue current route

## Recommendation

Continue by rerunning/classifying `src/20030125-1.c` through the RV64 object
route. That is the correct next step because the latest committed code moved
the route from raw FPR formal-home rejection toward prepared identity-backed
admission, and the existing representative log is stale: it still records the
old `unsupported_param_home: ... supported GPR homes` boundary from before
`b7e0a48df`.

Do not rewrite `plan.md` or `todo.md` for route reasons before that rerun. The
plan's Step 4 explicitly says to inspect `src/20030125-1.c`, implement narrow
ABI support only when argument/result/register-class facts exist, and document
representative outcomes without weakening expectations (`plan.md:137`,
`plan.md:149`, `plan.md:156`). The current `todo.md` Suggested Next already
matches that (`todo.md:30`).

No testcase overfit found. I did not find source-case name matching, source
pattern matching, expectation downgrades for `src/20030125-1.c`, raw `fa0`
fallback parsing, or printed-IR substring matching in the current FPR route.
