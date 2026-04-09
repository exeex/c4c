# Backend Surface Cleanup After MIR Migration

Status: Open
Last Updated: 2026-04-09

## Goal

Clean up backend header boundaries and target-facing API surfaces that are
worth fixing after the active typed-LIR/backend-ownership migration lands, but
that are not required to complete the current Step 6 and Step 7 work.

## Why This Idea Exists

The active plan is correctly focused on moving backend ownership off raw `LIR`
and onto the `BIR -> backend CFG/MIR -> machine-facing analysis` path.
While auditing `src/backend/*.hpp`, several cleanup opportunities showed up
that would improve maintainability for future targets such as RISC-V, but they
do not directly unblock the current migration and would add scope creep if
folded into `plan.md`.

This idea keeps those follow-ups visible without slowing the ownership move.

## Deferred Problem Surface

### 1. Backend header layering is still broader than necessary

- `src/backend/liveness.hpp` currently mixes backend CFG carrier types,
  liveness-lowering inputs, direct lowering entrypoints, and final liveness
  results in one public surface
- `src/backend/stack_layout/slot_assignment.hpp` currently exposes planning,
  prepared fallback metadata, rewrite patching, and module-level orchestration
  from one header

### 2. Scenario-specific helper surfaces should be separated from core lowering

- `src/backend/lowering/call_decode.hpp` currently mixes general call decoding
  helpers with many specialized matcher/view structs that look more like narrow
  pattern-recognition utilities than core lowering contracts

### 3. Target backend public APIs are repetitive and only partly normalized

- `src/backend/x86/codegen/emit.hpp` and
  `src/backend/aarch64/codegen/emit.hpp` expose nearly identical surfaces
- target-specific assembler and linker headers expose more internal structure
  than future backend bring-up likely needs
- `src/backend/target.hpp` already names targets such as `I686` and `Riscv64`
  whose capability/story is not yet expressed through a clear backend
  capability surface

### 4. Small IR/API consistency cleanups remain desirable

- `src/backend/bir.hpp` uses `std::variant` for instructions but a manual
  tagged payload for `Terminator`
- `src/backend/backend.hpp` uses an asymmetric `BackendModuleInput` ownership
  model that owns `bir::Module` but only borrows `LirModule`

## Proposed Follow-Up Scope

When this idea is activated, likely slices include:

1. narrow `src/backend/liveness.hpp` and `src/backend/stack_layout/*.hpp`
   public surfaces after the Step 6/7 ownership move stabilizes
2. split `src/backend/lowering/call_decode.hpp` into core decode utilities and
   specialized shape matchers
3. define a clearer target backend capability/interface story before or during
   broader RISC-V backend bring-up
4. apply small IR/API consistency cleanups such as `bir::Terminator` shape and
   backend module input ownership semantics

## Non-Goals

- changing the active source idea or mutating `plan.md` to absorb unrelated
  cleanup immediately
- blocking Step 6 or Step 7 on aesthetic header cleanup
- broad target backend abstraction before there is a concrete consumer for it

## Activation Hint

Activate this idea after the current typed-LIR/backend-ownership migration
reaches a stable Step 7 or close-plan point, or earlier only if one of the
listed cleanup items becomes a demonstrated blocker for a concrete new backend
slice.
