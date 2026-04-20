# Generic Scalar Instruction Selection For Prepared X86

Status: Active
Source Idea: ideas/open/59_generic_scalar_instruction_selection_for_x86.md
Activated from: ideas/closed/61_stack_frame_and_addressing_consumption.md

## Purpose

Turn idea 59 into an execution runbook that replaces prepared-x86 whole-function
matcher growth with ordinary per-operation and per-terminator instruction
selection over authoritative prepared inputs.

## Goal

Make x86 scalar lowering dispatch on prepared operations, operands, and
terminators so legality decisions are local and machine-shaped instead of being
encoded as bounded whole-function matcher families.

## Core Rule

Do not add new whole-function or named-testcase x86 matcher families. Repair
the lowering structure by consuming prepared CFG, value-home, move-bundle, and
addressing facts through ordinary selector helpers and per-op dispatch.

## Read First

- `ideas/open/59_generic_scalar_instruction_selection_for_x86.md`
- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- `src/backend/prealloc/prealloc.hpp`

## Scope

- define an x86 prepared-function/block context that exposes authoritative
  prepared inputs to scalar lowering
- extract legality-oriented selector helpers for scalar values, memory
  operands, compares, calls, and terminators
- migrate covered scalar operations and terminators onto per-op dispatch
- prove the route with build plus narrow backend/x86 coverage, then broaden
  when the dispatch blast radius grows

## Non-Goals

- introducing a separate machine IR in the same slice
- x86 peephole optimization
- target-independent SSA rewrites
- reopening prepared CFG ownership from idea 58, prepared value-home ownership
  from idea 60, or frame/address ownership from idea 61

## Working Model

- upstream prepared ownership already publishes CFG meaning, value homes,
  move obligations, and frame/address facts for the covered route
- x86 instruction selection should ask prepared data what an operation means,
  then choose a legal machine spelling
- selector helpers should answer operand and legality questions, not recover
  whole-program semantics from function shape
- routine execution should prefer small migrations of coherent instruction or
  terminator families over broad emitter rewrites

## Execution Rules

- prefer extracted selector helpers and per-op dispatch over growth in `try_*`
  whole-function matcher families
- keep legality decisions target-local and semantic ownership upstream
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for each code slice
- escalate to broader backend validation when a slice changes shared selector
  helpers or several narrow instruction-family packets have landed

## Step 1: Establish Prepared Dispatch Surface

Goal: define the x86 prepared-function and block context plus the top-level
dispatch boundaries needed for ordinary scalar lowering.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- identify or extract one authoritative prepared-function context surface that
  can hand scalar lowering the current function, block, control-flow,
  value-home, move-bundle, and addressing views
- separate top-level function, block, instruction, and terminator emission
  responsibilities so later packets can migrate families without whole-function
  matcher coupling
- keep this step structural; do not widen it into full family migration yet

Completion check:

- the x86 prepared route has a visible dispatch surface that later packets can
  target without first re-deriving whole-function matcher structure

## Step 2: Extract Operand And Legality Selectors

Goal: provide reusable selector helpers for the prepared scalar route.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- helper extraction sites under x86 codegen if needed

Actions:

- extract or refine selector helpers for named/immediate scalar values, stack
  and symbol memory operands, prepared compare planning, and bounded call-lane
  legality
- keep helper contracts framed around prepared inputs and x86 legality, not
  testcase-shaped whole-function success conditions
- preserve upstream ownership boundaries: do not re-derive CFG meaning, value
  homes, or frame/address provenance locally

Completion check:

- covered scalar lowering questions can be answered through local selector
  helpers instead of by growing another bounded whole-function matcher

## Step 3: Migrate Covered Scalar Instruction Families

Goal: move the covered scalar instruction families onto per-operation
instruction selection.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`
- focused backend/x86 proof coverage matched to each migrated family

Actions:

- migrate binary, load/store, select, and related covered scalar instruction
  families through per-op dispatch and selector helpers
- keep each packet bounded to one coherent family or adjacent helper surface
- delete or isolate matcher-only branches only when the replacement path is
  already driven by prepared per-op dispatch

Completion check:

- the covered scalar instruction families are emitted through ordinary
  instruction selection over prepared inputs, not through whole-function shape
  recognition

## Step 4: Migrate Covered Terminator And Call Families

Goal: move the covered branch and call emission paths onto prepared
per-terminator and per-call selection.

Primary targets:

- `src/backend/mir/x86/codegen/prepared_module_emit.cpp`
- `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`

Actions:

- migrate covered compare-and-branch, boolean-branch, and scalar call families
  onto prepared control-flow, move-bundle, and operand selectors
- keep unsupported families explicitly unsupported rather than adding new
  matcher-shaped acceptance paths
- stop when the remaining x86-only decisions are machine legality and spelling,
  not whole-function semantic recovery

Completion check:

- the covered branch and call routes dispatch through prepared per-terminator
  or per-call selection instead of bounded whole-function x86 matchers

## Step 5: Validate The Route

Goal: prove the new scalar instruction-selection structure without relying on
one named testcase.

Actions:

- require a fresh build for every accepted slice
- choose the narrowest proving subset that exercises the migrated instruction
  or terminator family
- broaden validation when shared selector helpers or dispatch boundaries change
  across multiple backend buckets, or when the route is being treated as a
  milestone
- reject slices whose practical effect is matcher growth or expectation
  weakening rather than selector-based capability repair

Completion check:

- accepted slices have fresh proof logs and validation proportional to the
  scalar-lowering blast radius
