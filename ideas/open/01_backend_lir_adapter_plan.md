# Backend LIR Adapter Plan

## Relationship To Roadmap

Umbrella source: `ideas/open/__backend_port_plan.md`

This is the first backend execution target.

## Goal

Create the narrowest possible attachment layer that lets a C++ port of `ref/claudes-c-compiler/src/backend/` consume our existing LIR without forcing an early backend redesign.

## Why This Needs Its Own Idea

The biggest intentional difference from ref is the input IR boundary.

If that difference is not isolated early, backend bring-up risks turning into:

- string parsing scattered through codegen
- accidental LIR redesign
- target codegen mixed with adapter logic

This idea exists to keep that risk contained.

## Strategy

- port the ref backend shape as mechanically as possible
- introduce a dedicated adapter/shim layer where our LIR differs from ref expectations
- keep backend core code as close to ref behavior and organization as possible

## Scope

- create `src/backend/` skeleton and top-level entry wiring
- define target selection and backend factory boundaries
- define the dispatch contract from `LirModule` / `LirInst` into backend codegen
- decide how Stage 3 LIR is handled in the first version
- keep any Stage 3 parsing or translation inside a clearly bounded adapter layer
- wire external assembler/linker fallback for first executable bring-up
- add CMake integration and minimal tests

## Explicit Non-Goals

- full AArch64 instruction coverage
- register allocation
- built-in assembler
- built-in linker
- reworking `stmt_emitter.cpp` to emit a different LIR flavor

## Key Decisions To Lock Down

### 1. Backend input contract

Define the internal backend-facing representation expected after LIR adaptation.

The first version may still be thin and imperfect, but it should be explicit.

### 2. Stage 3 handling

Choose one clear initial policy:

- adapt Stage 3 directly through a shim
- normalize Stage 3 to a backend-local structured form

Whichever path is chosen, the logic should stay outside target-specific codegen.

### 3. External tool fallback

Define one stable contract for first bring-up:

- tool discovery
- target triple / cross tool invocation
- failure reporting
- test harness expectations

## Validation

- `int main() { return 0; }` produces a working executable through the backend entry
- `return 2 + 3` produces exit code `5`
- unit test coverage exists for target factory / dispatch entry
- tests clearly separate adapter failures from target codegen failures

## Good First Patch

Add `src/backend/` skeleton, target enum/factory, backend entry point, and a minimal LIR dispatch path that can lower a trivial return-only program through external tool fallback.
