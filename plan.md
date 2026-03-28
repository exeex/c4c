# AArch64 Backend Bring-up Runbook

Status: Active
Source Idea: ideas/open/02_backend_aarch64_port_plan.md

## Purpose

Turn the AArch64 backend port idea into a narrow execution runbook focused on compile integration first, backend-driver/LIR attachment second, and hello-world-level assembly emission third.

## Goal

Make the mirrored shared backend and AArch64 backend tree participate in the real build, reconnect the AArch64 entry path to the current backend/LIR boundary with the smallest necessary shim, and reach a hello-world-level `.s -> .o` proof using the existing external `llvm-mc` flow.

## Core Rule

Preserve the ref backend's file and mechanism boundaries wherever practical; prefer mechanical translation, thin temporary shims, and build-reachable slices over broad redesign or early optimization work.

## Read First

- `ideas/open/02_backend_aarch64_port_plan.md`
- `ideas/open/__backend_port_plan.md`
- `ref/claudes-c-compiler/src/backend/`
- `src/backend/`
- `src/backend/aarch64/`
- current backend-driver and LIR attachment surfaces under `src/codegen/` and `src/backend/`

## Current Targets

- Make mirrored shared backend layers compile together as C++:
  `generation`, `state`, `traits`, `liveness`, `regalloc`, `stack_layout`, `elf`, `linker_common`
- Make mirrored AArch64-local layers compile together:
  `codegen`, `assembler`, `linker`
- Reconnect the backend entry path so AArch64 is build-reachable from the normal backend driver
- Keep the LIR attachment layer explicit and narrow
- Switch the minimal supported AArch64 path from LLVM-text fallback to `.s` emission
- Prove one hello-world-level `.s` can be assembled into an ELF object through external `llvm-mc`

## Non-Goals

- x86-64 or rv64 work
- built-in assembler implementation
- built-in linker implementation
- broad LIR redesign
- premature register-allocation or peephole cleanup work beyond what is required for the initial AArch64 slice
- case-by-case runtime feature chasing before the mirrored backend tree is actually wired into the build

## Working Model

- Shared backend compile integration comes before behavior tightening
- AArch64-local compile integration comes before backend-driver wiring
- Backend-driver wiring comes before broader instruction/ABI completeness
- Initial bring-up may be stack-first and spill-heavy
- Temporary shims are acceptable when they preserve ref-shaped boundaries and keep future porting local

## Execution Rules

- Do not silently expand this runbook into separate assembler/linker initiatives; record truly separate work in `ideas/open/`
- Prefer one thin vertical slice at a time
- Keep new abstractions narrow and justified by the current slice
- When blocked by interface mismatch, add the smallest adapter layer instead of mutating existing LIR structures broadly
- Treat compile-time reachability and monotonic build progress as first-class validation, not only runtime tests

## Step 1: Inventory And Compile The Shared Backend Slice

Goal: identify the smallest shared backend module set that must compile together for the AArch64 port to become build-participating.

Primary target:
- `src/backend/`

Actions:
- inspect the mirrored shared backend files already present under `src/backend/`
- compare them against the ref shared backend surfaces named in the source idea
- add or fix the minimum include edges, declarations, namespaces, forward declarations, and type shims needed for a shared compile-integrated slice
- keep the resulting boundary close to ref naming and file ownership

Completion check:
- a bounded shared backend slice under `src/backend/` compiles as C++ in the real build or in a build-reachable library/unit without placeholder-only disconnection

## Step 2: Inventory And Compile The AArch64-Local Slice

Goal: make the mirrored AArch64 subtree compile together against the shared slice.

Primary target:
- `src/backend/aarch64/`

Actions:
- inspect the mirrored AArch64-local files for `codegen`, `assembler`, and `linker`
- wire their includes and declarations to the shared backend slice from Step 1
- preserve target-local ownership for AArch64-specific codegen and toolchain-facing logic
- keep built-in assembler and linker work at compile-integration level only if touched at all

Completion check:
- a bounded AArch64-local backend slice compiles together with the shared backend slice and is no longer isolated scaffolding

## Step 3: Reconnect Backend Entry Points

Goal: make the AArch64 path build-reachable from the normal backend driver.

Primary target:
- backend entry points and driver wiring under `src/backend/` and `src/codegen/`

Actions:
- identify the current backend-driver entry path and fallback behavior
- reconnect top-level AArch64 backend entry points, namespaces, and declarations
- eliminate placeholder boundaries that prevent the mirrored backend from being instantiated normally
- keep the wiring narrow and explicit

Completion check:
- the normal backend driver can instantiate or route into the AArch64 path without defaulting immediately to the old LLVM-text-only route

## Step 4: Attach The Current LIR Boundary With The Smallest Shim

Goal: make the current LIR input consumable by the mirrored backend path through an explicit adapter layer.

Primary target:
- LIR-facing backend boundary under `src/codegen/` and `src/backend/`

Actions:
- inspect the existing LIR structures and the ref backend expectations
- define the narrowest adapter/shim layer needed to bridge mismatches
- avoid redesigning core LIR data structures unless strictly required by the active slice
- keep the attachment boundary discoverable in code

Completion check:
- the LIR-to-backend connection is explicit, narrow, and sufficient for the initial AArch64 bring-up slice

## Step 5: Emit Minimal AArch64 Assembly

Goal: switch the initial supported AArch64 path to textual assembly emission.

Primary target:
- AArch64 codegen/emission path

Actions:
- route the active AArch64 backend slice to emit `.s` instead of backend-specific LLVM text
- start with a hello-world-level supported subset only
- accept conservative stack-first code shape if it keeps correctness and port structure clear
- keep unsupported behavior explicit instead of hiding it behind silent fallbacks

Completion check:
- a hello-world-level supported case produces AArch64 `.s` from the ref-shaped backend path

## Step 6: Prove External Assembly To ELF Object

Goal: validate that the emitted assembly is accepted by the current external assembler flow.

Primary target:
- external `llvm-mc` toolchain path

Actions:
- run the existing external assembly flow on the emitted `.s`
- adjust the minimal emitted syntax only where required for acceptability
- keep the toolchain wrapper deterministic and boring

Completion check:
- at least one hello-world-level AArch64 `.s` assembles into an ELF `.o` through the existing external `llvm-mc` path

## Step 7: Tighten Behavior In Ref Order

Goal: continue feature work only after compile integration, wiring, and `.s -> .o` proof exist.

Primary target:
- AArch64 codegen behavior

Actions:
- continue in ref order:
  `emit`, `prologue`, `memory`, `alu`, `comparison`, `calls`, `globals`, `returns`
- keep follow-on regalloc/cleanup work aligned with `ideas/open/03_backend_regalloc_peephole_port_plan.md`
- treat new separate initiatives as new/open ideas instead of mutating this runbook broadly

Completion check:
- only enter this step after Steps 1 through 6 are demonstrably complete

## Validation

- more mirrored backend files participate in the real build over time
- the AArch64 path is reachable from the normal backend driver
- the LIR attachment boundary is explicit and narrow
- hello-world-level AArch64 cases emit `.s`
- emitted `.s` is accepted by the existing external `llvm-mc` path to produce ELF `.o`
- full-suite regressions remain monotonic when code changes begin under this runbook

## Acceptance

This runbook is complete when:

- the mirrored ref-shaped backend tree is a build-participating C++ subsystem
- AArch64 backend entry points are wired through the normal backend driver
- the initial LIR attachment boundary is explicit and narrow
- a minimal supported AArch64 slice emits `.s`
- the existing external `llvm-mc` path assembles that `.s` into an ELF object

Broad runtime completeness, full ABI coverage, built-in assembler work, and built-in linker work remain outside this runbook.
