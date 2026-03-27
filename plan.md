# AArch64 Backend Port Runbook

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md
Activated from: ideas/open/02_backend_aarch64_port_plan.md

## Purpose

Port the AArch64 backend from `ref/claudes-c-compiler/src/backend/` into C++ while keeping structure and behavior as close to ref as the existing LIR adapter boundary allows.

## Goal

Bring the AArch64 path from the current return-only backend entry to a mechanically ported integer/control-flow slice that can execute through the existing external toolchain fallback.

## Core Rule

Prefer mechanical translation from ref backend code and keep ABI, frame, and instruction-selection decisions explicit inside `src/backend/` rather than leaking target logic into the LIR adapter or driver.

## Read First

- `ideas/open/02_backend_aarch64_port_plan.md`
- `ideas/open/__backend_port_plan.md`
- `ref/claudes-c-compiler/src/backend/`
- `src/backend/`
- `tests/c/internal/backend_case/`

## Scope

- port the first AArch64 target-specific backend files and helper boundaries into `src/backend/`
- support integer arithmetic, loads/stores, compares, branches, direct calls, returns, and global addressing for the first useful slice
- preserve the existing adapter boundary and external toolchain fallback
- validate on the supported AArch64 Linux target path

## Non-Goals

- x86-64 or rv64 implementation work
- built-in assembler or linker
- broad LIR redesign
- opportunistic backend cleanup outside the AArch64 port slice

## Working Model

- keep target-independent contracts in the existing backend boundary
- port AArch64 mechanisms from ref as directly as possible before optimizing structure
- treat ABI fixes and calling-convention work as explicit backend tasks, not incidental follow-on edits
- keep the host-vs-target toolchain path deterministic and easy to diagnose

## Execution Rules

- keep each patch narrow and test-backed
- compare against ref backend structure before inventing new abstractions
- do not hide target-specific decisions in `src/codegen/llvm/`
- record separate follow-on initiatives in `ideas/open/` instead of silently expanding scope

## Ordered Steps

### Step 1: Inspect the ref AArch64 backend surfaces

Goal: identify the minimum file set and helper boundaries that must be ported first.

Primary targets:

- `ref/claudes-c-compiler/src/backend/`
- `src/backend/`

Concrete actions:

- inspect the ref AArch64 backend entry, frame helpers, and instruction-emission flow
- map the ref file layout onto the current C++ backend boundary
- record the minimum first port slice for return, integer arithmetic, and branch bring-up

Completion check:

- the active plan names the exact ref files and new `src/backend/` files that define the first AArch64 port boundary

### Step 2: Add AArch64 backend scaffolding

Goal: create the target-specific C++ structure without broadening behavior yet.

Primary targets:

- `src/backend/`
- build-system wiring

Concrete actions:

- add AArch64-specific backend files and entry hooks under `src/backend/`
- keep target selection and factory wiring explicit
- make unsupported operations fail narrowly and diagnostically

Completion check:

- the tree builds with the new AArch64 backend skeleton and no target logic spread outside the backend subsystem

### Step 3: Port the first integer/control-flow slice

Goal: lower beyond return-only programs for the initial AArch64 bring-up.

Primary targets:

- AArch64 instruction emission
- frame and register-use helpers

Concrete actions:

- port prologue/epilogue and naive frame handling from ref
- port integer arithmetic, compare, branch, direct-call, and return paths needed by current tests
- keep global addressing and stack-slot handling explicit and minimally sufficient

Completion check:

- arithmetic, branch, and simple multi-call cases reach backend emission through the AArch64 target path

### Step 4: Validate the external execution path

Goal: prove the AArch64 backend slice works through the agreed external toolchain contract.

Primary targets:

- backend runtime tests
- external toolchain invocation path

Concrete actions:

- map the supported AArch64 Linux target flow onto deterministic assembler/linker execution
- add or update runtime tests for arithmetic, branch, recursion, and multi-call coverage
- keep toolchain, backend, and runtime failures attributable

Completion check:

- the supported AArch64 Linux path executes the intended minimal subset through backend output plus external toolchain fallback

## Exit Criteria

- AArch64 target-specific backend files exist under `src/backend/`
- the first useful integer/control-flow slice is ported mechanically from ref
- runtime validation covers more than return-only execution
- the external toolchain path remains explicit and attributable for the supported AArch64 flow
