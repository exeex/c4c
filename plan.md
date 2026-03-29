# Backend X86 External Call Object Runbook

Status: Active
Source Idea: ideas/open/33_backend_x86_external_call_object_plan.md
Activated from: ideas/open/__backend_port_plan.md

## Purpose

Drive the next bounded x86 backend slice that keeps `call_helper.c` on the
backend-owned asm/object path and proves the external-call object contract.

## Goal

Produce backend-owned x86 assembly/object output for `call_helper.c` that emits
`call helper` and preserves the unresolved external-call relocation expected for
an x86-64 Linux object build.

## Core Rule

Keep this slice limited to the direct external-call object contract for
`tests/c/internal/backend_case/call_helper.c`. Do not broaden into general x86
linker bring-up or unrelated runtime convergence work.

## Read First

- [`ideas/open/33_backend_x86_external_call_object_plan.md`](/workspaces/c4c/ideas/open/33_backend_x86_external_call_object_plan.md)
- [`tests/c/internal/backend_case/call_helper.c`](/workspaces/c4c/tests/c/internal/backend_case/call_helper.c)
- [`tests/c/internal/InternalTests.cmake`](/workspaces/c4c/tests/c/internal/InternalTests.cmake)
- [`tests/c/internal/cmake/run_backend_contract_case.cmake`](/workspaces/c4c/tests/c/internal/cmake/run_backend_contract_case.cmake)
- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp)

## Current Targets

- keep `backend_runtime_call_helper` on `BACKEND_OUTPUT_KIND=asm`
- add or enable a focused x86 contract test that inspects backend-owned
  assembly/object output for `call_helper.c`
- make the smallest x86 backend change needed so the case no longer falls back
  to LLVM text

## Non-Goals

- no umbrella-level planning work
- no unrelated x86 runtime-case migrations
- no broad relocation/linker redesign
- no target-general backend cleanup outside the direct-call slice

## Working Model

Treat Clang plus `objdump -r` as the relocation reference. Implement the
smallest x86 target-local change that preserves already-working backend-owned
asm cases while making the external helper call visible in both emitted
assembly and the assembled object.

## Execution Rules

- add the narrowest validating test before implementation
- keep x86-specific logic isolated to the x86 backend surfaces unless the test
  proves a shared helper is the root cause
- if the missing behavior turns out to require broader external symbol handling,
  record that as a separate idea instead of silently expanding this runbook
- update `plan_todo.md` at each slice boundary so the next agent can resume
  without re-deriving the state

## Ordered Steps

### Step 1: Lock the failing x86 contract

Goal: capture the exact current gap for backend-owned x86 asm/object output.

Primary Target:

- [`tests/c/internal/InternalTests.cmake`](/workspaces/c4c/tests/c/internal/InternalTests.cmake)
- [`tests/c/internal/cmake/run_backend_contract_case.cmake`](/workspaces/c4c/tests/c/internal/cmake/run_backend_contract_case.cmake)

Actions:

- record the current baseline and reproduce `backend_runtime_call_helper`
- add or enable an x86 contract test for `call_helper.c` object output
- capture the current backend assembly/object output and compare it with Clang
  plus `objdump -r`

Completion Check:

- one targeted x86 contract/runtime test pair exposes the current external-call
  object gap in a repeatable way

### Step 2: Keep the case on the backend-owned x86 asm path

Goal: emit backend-owned x86 assembly for the direct helper call without
regressing simpler asm-backed runtime cases.

Primary Target:

- [`src/backend/x86/codegen/emit.cpp`](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [`src/backend/x86/codegen/calls.cpp`](/workspaces/c4c/src/backend/x86/codegen/calls.cpp)

Actions:

- inspect the minimal direct/external call slice selection already present in
  the x86 backend
- implement the smallest missing x86 target-local logic required for
  `call_helper.c`
- preserve backend-owned asm behavior for existing green x86 runtime cases such
  as `return_zero.c` and `return_add.c`

Completion Check:

- `call_helper.c` stays on backend-owned x86 asm output and the emitted assembly
  contains the expected direct helper call

### Step 3: Prove object and suite behavior

Goal: verify relocation/object behavior and confirm no regression in the wider
test suite.

Primary Target:

- targeted backend runtime and contract tests
- full `ctest` suite

Actions:

- assemble the x86 backend output and inspect relocations with `objdump -r`
- confirm the unresolved helper symbol and expected relocation form are present
- rerun nearby backend tests and then the full suite
- update `plan_todo.md` with completion state, remaining blockers, or the next
  bounded follow-up

Completion Check:

- the targeted x86 contract/runtime checks pass and the full suite remains
  monotonic
