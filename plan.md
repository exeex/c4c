# AArch64 Call And Frame Machine Nodes Runbook

Status: Active
Source Idea: ideas/open/231_aarch64_call_frame_machine_nodes.md
Activated from: ideas/open/231_aarch64_call_frame_machine_nodes.md

## Purpose

Lower existing prepared AArch64 call and frame authority into selected machine
nodes and printable prologue, epilogue, and call output without recreating ABI
classification in target codegen.

## Goal

Consume `PreparedCallPlan`, call-boundary move bundles, ABI bindings,
preserved-value records, clobber records, and `PreparedFramePlanFunction`
facts to emit structured AArch64 call/frame machine nodes.

## Core Rule

AArch64 codegen must consume prepared/shared facts. Do not invent outgoing
stack areas, callee-save slots, sret locations, scratch registers, or ABI
classification decisions inside target lowering or the terminal printer.

## Read First

- `ideas/open/231_aarch64_call_frame_machine_nodes.md`
- AArch64 lowering dispatch for `bir::CallInst`
- Prepared call and frame plan definitions:
  `PreparedCallPlan`, `PreparedFramePlanFunction`, `BeforeCall`, `AfterCall`
- AArch64 selected machine-node records and terminal printer surfaces
- Existing tests or fixtures that exercise direct calls, indirect calls,
  callee-save frame setup, and memory-return calls

## Current Targets

- AArch64 call machine-node lowering from prepared call facts
- AArch64 prologue/epilogue lowering from prepared frame facts
- Printer support for selected call, frame setup, frame teardown, and
  save/restore nodes whose structured operands are complete
- Narrow proof for fixed-arity direct calls, indirect calls, callee-save
  frames, and memory-return calls

## Non-Goals

- Do not reconstruct archived `calls.md` or `prologue.md` local ABI
  classifiers.
- Do not implement full variadic function-entry `va_list` behavior here.
- Do not create local stack-slot allocation, spill authority, or call
  preservation policy in AArch64 codegen.
- Do not add named-case call/frame shortcuts or weaken tests to show progress.
- Do not absorb unrelated scalar ALU, memory load/store, global address,
  intrinsic, inline-asm, i128, or binary128 routes.

## Working Model

Prepared/shared layers own ABI and frame authority. The AArch64 backend should:

1. inspect available prepared facts and fail explicitly when required facts are
   missing;
2. lower those facts into typed selected machine records;
3. print only records with complete structured operands;
4. prove behavior with focused call/frame cases before broader c-testsuite
   reruns.

## Execution Rules

- Keep routine packet progress and proof in `todo.md`.
- Prefer small packets: inspect facts, introduce records, lower one family,
  print one family, then validate.
- Treat expectation-only updates, unsupported downgrades, or case-name matching
  as route drift.
- When a missing prepared fact blocks lowering, record the blocker and create
  or request a separate source idea instead of fabricating target-local state.
- For code-changing packets, prove with build plus the narrow call/frame subset
  chosen by the supervisor; escalate to backend or AArch64 c-testsuite subsets
  when call/frame behavior affects shared backend output.

## Ordered Steps

### Step 1: Inspect Prepared Facts And Current Dispatch

Goal: Identify the exact prepared call/frame facts available to AArch64
lowering and where current dispatch stops.

Primary targets:

- `PreparedCallPlan` and related `BeforeCall` / `AfterCall` move bundles
- `PreparedFramePlanFunction`
- AArch64 block dispatch for `bir::CallInst`
- Existing selected machine-record and printer definitions

Actions:

- Inspect how direct calls, indirect calls, memory-return calls, preserved
  values, clobbers, and call-boundary moves are represented today.
- Inspect how frame size, alignment, callee-save registers, frame slots,
  dynamic-stack state, and frame-pointer policy are represented today.
- Identify missing target machine-record fields before writing implementation
  code.
- Pick representative narrow proof cases for the first lowering packet.

Completion check:

- `todo.md` records the concrete lowering/printer surfaces to edit, the proof
  subset to run first, and any missing prepared facts that must not be
  invented in AArch64 codegen.

### Step 2: Add Call And Frame Machine Records

Goal: Make selected machine records capable of carrying prepared call/frame
facts without relying on rendered text or local ABI reconstruction.

Actions:

- Add records for direct and indirect call emission, call result collection,
  memory-return handling, preserved-value moves, and clobber observations as
  needed by the inspected facts.
- Add records for prologue setup, epilogue teardown, callee-save save/restore,
  and frame-pointer policy as needed by prepared frame facts.
- Keep record fields typed and structured enough for the printer to reject
  incomplete nodes.

Completion check:

- Machine records can represent the call/frame proof cases without ad hoc
  string payloads or target-local ABI decisions.

### Step 3: Lower Simple Prepared Frame Facts

Goal: Emit simple fixed-frame prologue and epilogue machine nodes from
`PreparedFramePlanFunction` and prepared frame-slot facts without callee-save
save/restore placement inference.

Actions:

- Lower frame size, alignment, frame-pointer policy, and frame-slot order into
  selected frame nodes for no-save fixed-frame cases.
- Preserve saved callee-register facts as structured data, but do not emit
  callee-save save/restore records until an explicit prepared
  saved-register-to-frame-slot mapping exists.
- Reject or defer dynamic-stack states whose prepared facts are incomplete.
- Add printer support for the completed prologue/epilogue subset.

Completion check:

- Simple fixed-frame functions emit structured prologue and epilogue output
  from prepared frame facts, and callee-save lowering is deferred to
  `ideas/open/241_prepared_callee_save_slot_placement.md` instead of inferred
  locally.

### Step 4: Lower Direct And Indirect Calls

Goal: Emit AArch64 call nodes from `PreparedCallPlan` and call-boundary move
bundles.

Actions:

- Lower direct fixed-arity calls from prepared callee identity, argument
  bindings, move bundles, result facts, clobbers, and preserved values.
- Lower indirect calls by consuming the prepared indirect callee carrier.
- Add printer support for selected call nodes and required move/result
  collection nodes.
- Keep variadic call-boundary minimum separate from full variadic function
  entry.

Completion check:

- Direct fixed-arity calls and indirect calls emit structured AArch64 output
  without hard-coded spill or scratch conventions.

### Step 5: Lower Memory-Return And Preservation Cases

Goal: Consume prepared sret storage, result records, preserved-value records,
and clobber records for call boundaries.

Actions:

- Lower memory-return calls using prepared storage and result facts.
- Preserve values across calls only according to prepared preservation records.
- Reflect clobber records in selected nodes or diagnostics where relevant.
- Add focused proof for a memory-return call and a preserved-value call case.

Completion check:

- Memory-return and preservation proof cases emit valid structured machine
  output and do not reconstruct ABI policy in AArch64 lowering.

### Step 6: Validate And Summarize

Goal: Prove the call/frame route and preserve remaining limitations at the
right lifecycle layer.

Actions:

- Run the supervisor-chosen build and narrow call/frame proof subset.
- Escalate to backend or AArch64 c-testsuite subsets if the changed surfaces
  affect broader backend output.
- Record remaining unsupported call/frame states in `todo.md`.
- Do not close the source idea until the callee-save placement split is either
  completed and reintegrated or the supervisor explicitly accepts a narrower
  source-idea completion boundary.
- Ask the supervisor whether the source idea is complete, blocked, or should
  be split before close.

Completion check:

- The direct call, indirect call, callee-save frame, and memory-return proof
  directions are either green with structured output or have explicit
  non-overfit follow-up blockers recorded.
