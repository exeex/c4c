# ABI Call Result And Stack-Frame Lowering

Status: Closed
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: ABI/RV64 consumer
Queue Order: 12
Prerequisites: local-memory and global-data producer prerequisites must be explicit; prepared call/return facts must exist before RV64 consumption
Estimated Evidence Breadth: `60` ABI/RV64 rows
Proof Surface: ordinary same-module call ABI/result, stack-frame layout, and return move-bundle target rows

## Goal

Repair ordinary call, result, return, and stack-frame RV64 lowering where the
prepared call/return facts are available and the first owner is ABI/RV64
consumption.

## Why This Exists

The failure map shows `46` ordinary call ABI/result rows, `12` stack-frame
layout rows, and `2` return move-bundle target rows. These should run after
local/global memory prerequisites are clear.

## Closure Summary

Closed after the active runbook reached Step 5 residual split/close-readiness
classification. The route repaired ordinary same-module ABI/RV64 consumer
cases that had explicit prepared authority, including scalar GPR call/result
transport and address-provenance-backed register/frame-slot argument sources.
The final refreshed residual scan found no remaining complete-authority
ABI/RV64 consumer family with meaningful breadth.

Durable residual work was split to producer or policy ideas instead of being
inferred in RV64 object emission:

- `ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`
  covers outgoing stack destination offsets and broader aggregate/outgoing
  stack argument transport authority.
- `ideas/open/625_prepared_stack_slot_preservation_source_publication.md`
  covers prepared stack-slot preservation source endpoints.
- `ideas/open/626_prepared_dynamic_frame_callee_saved_slot_placement.md`
  covers prepared GPR dynamic-frame callee-saved save-slot placements.
- `ideas/open/627_pointer_stack_result_call_policy.md` covers pointer-valued
  call results whose destination is a stack slot or aggregate home.
- `ideas/open/628_fpr_abi_frame_policy_and_placement.md` covers FPR ABI,
  floating frame policy, and prepared FPR frame placement authority.
- `ideas/open/629_prepared_return_destination_home_authority.md` covers
  prepared return destination-home publication for return move-bundle rows.

The closed idea should not be reopened merely because residual rows still carry
`unsupported_call_abi` or related labels. Those rows now require explicit
producer/policy authority before RV64 consumer lowering can remain semantic.

## In Scope

- Ordinary same-module call and result lowering.
- Supported stack-frame layout consumption.
- Return move-bundle target handling when upstream facts are present.
- Guardrails for variadic, library, runtime, or missing-authority rows.

## Out Of Scope

- Variadic/library call policy.
- Runtime mismatch triage.
- BIR local/global producer repairs, prepared authority production,
  expectations, unsupported markers, allowlists, timeouts, or accounting.

## Acceptance Criteria

- Multiple ordinary call/result/frame rows progress.
- Runtime, library, variadic, and missing-authority rows remain separated.
- Proof includes at least one call/result row and one stack-frame or return
  row when available.

## Reviewer Reject Signals

- Reject final-assembly inference when prepared call/return facts are missing.
- Reject merging runtime abort/segfault fixes into this ABI consumer idea.
- Reject named-case-only call lowering.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes as progress.
- Reject retaining the same unsupported ABI shape under renamed helpers.
