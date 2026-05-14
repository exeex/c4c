# AArch64 Call And Frame Machine Nodes

Status: Closed
Created: 2026-05-14
Closed: 2026-05-14

Parent Context: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md

## Problem

The AArch64 backend has structured prepared authority for calls and frames, but
compiled codegen does not yet lower that authority into selected machine nodes.
`PreparedCallPlan` carries direct/indirect call identity, arguments, result,
memory-return, preserved values, clobbers, wrapper kind, and minimal variadic
call-boundary metadata. `PreparedFramePlanFunction` carries frame size,
alignment, saved callee registers, frame-slot order, dynamic-stack state, and
frame-pointer policy.

Current AArch64 block dispatch still classifies `bir::CallInst` as the call
family without lowering it, and the terminal printer does not include a call
or prologue/epilogue printable subset. This leaves call emission, argument
movement consumption, result collection, callee-save saves/restores, and stack
frame setup/teardown as real AArch64 codegen gaps.

## Scope

- Lower AArch64 call machine nodes from `PreparedCallPlan`, `BeforeCall` /
  `AfterCall` move bundles, ABI bindings, memory-return plans, preserved-value
  records, and clobber records.
- Lower AArch64 frame/prologue/epilogue nodes from `PreparedFramePlanFunction`
  and prepared frame-slot facts.
- Keep ABI classification, stack-slot allocation, spill authority, and call
  preservation decisions in prepared/shared layers.
- Add printer support only for selected machine nodes whose structured facts
  are complete.

## Non-Goals

- Do not reconstruct the archived `calls.md` or `prologue.md` local ABI
  classifier.
- Do not invent outgoing stack areas, scratch registers, callee-save slots, or
  sret locations inside AArch64 codegen.
- Do not add named-case call/frame shortcuts or weaken tests to make narrow
  call examples pass.
- Do not implement full variadic function-entry `va_list` behavior here.

## Proof Direction

- A direct fixed-arity call emits structured AArch64 call output from prepared
  call facts.
- An indirect call consumes the prepared indirect callee carrier instead of a
  hard-coded spill convention.
- A function with saved callee registers emits matched prologue/epilogue
  saves/restores from prepared frame facts.
- A memory-return call consumes prepared sret storage and result records.

## Closure Summary

Closed under the implemented prepared-fact boundary after Step 6 validation.
The backend proof was:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_')`

CTest passed 139/139. The close-time regression guard passed in documented
non-decreasing mode for this validation/todo-only milestone:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

The guard reported before 139/0/139 and after 139/0/139, with no new failures
and no new slow tests.

Durable follow-up scope remains open in separate prepared-authority ideas:
`ideas/open/241_prepared_callee_save_slot_placement.md` and
`ideas/open/242_prepared_stack_slot_preserved_value_extent.md`.
