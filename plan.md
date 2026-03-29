# Backend Binary Utils Contract Runbook

Status: Active
Source Idea: ideas/open/04_backend_binary_utils_contract_plan.md

## Purpose

Create the compatibility baseline for the backend binary-utils track before built-in assembler or linker work starts.

## Goal

Document and lock the current AArch64 external-toolchain `.s -> .o -> executable` contract so later built-in replacements target a concrete, repo-local acceptance surface.

## Core Rule

Do not implement a built-in assembler or linker in this plan. Only establish compile-time/shared-interface readiness, document the current contract, and capture representative golden behavior.

## Read First

- [ideas/open/04_backend_binary_utils_contract_plan.md](/Users/chi-shengwu/c4c/ideas/open/04_backend_binary_utils_contract_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- `ref/claudes-c-compiler/src/backend/README.md`
- `ref/claudes-c-compiler/src/backend/common.rs`
- `ref/claudes-c-compiler/src/backend/arm/asm_stub.sh`
- `ref/claudes-c-compiler/src/backend/arm/ld_stub.sh`
- `ref/claudes-c-compiler/src/backend/arm/assembler/README.md`
- `ref/claudes-c-compiler/src/backend/arm/linker/README.md`
- `ref/claudes-c-compiler/src/backend/elf/`
- `ref/claudes-c-compiler/src/backend/linker_common/`

## Current Targets

- Identify the exact boundary between backend assembly emission and the external assembler/linker path used by the AArch64 backend.
- Declare or stage any shared ELF/archive/object/linker-facing contract surfaces needed for later built-in binary-utils work to compile cleanly.
- Capture at least one tiny single-object flow and one small multi-object relocation-bearing flow as the contract baseline.
- Record the assembly constructs, section/symbol expectations, relocation shapes, and executable entry assumptions exercised by those flows.
- Add focused fixtures or tests that let later plans compare behavior against the same baseline instead of rediscovering expectations.

## Non-Goals

- Built-in assembler implementation
- Built-in linker implementation
- Broad backend refactors unrelated to the external toolchain boundary
- Expanding supported instruction or relocation coverage beyond what current AArch64 backend output already requires for baseline cases

## Working Model

- Treat this plan as a contract-and-acceptance runbook for the binary-utils track.
- Preserve the current external-toolchain flow as the reference behavior.
- Prefer small, inspectable fixture cases over broad speculative documentation.
- Keep durable compatibility notes in the source idea if execution uncovers follow-on work not required to finish this baseline.

## Execution Rules

- Keep changes behavior-preserving unless a narrow compile-integration seam must be declared explicitly.
- Trace every documented requirement back to a concrete emitted assembly case, produced object file, linked executable, or reference backend surface.
- Separate backend-emission facts from assembler behavior and linker behavior so later failures can be attributed cleanly.
- Avoid creating ad hoc duplicate declarations if a bounded shared contract surface can be named once and reused later.

## Step 1: Inventory The Active Toolchain Boundary

Goal: identify the exact `.s -> .o -> executable` path and the concrete hand-off surfaces used by the current AArch64 backend.

Primary targets:

- backend driver entry points that invoke assembly and linking
- external-toolchain wrapper or stub surfaces
- any existing shared declarations later binary-utils code must include

Actions:

- inspect the current C++ AArch64 backend path and the mirrored reference backend path side by side
- record where assembly text is produced, where assembler invocation happens, and where linker invocation happens
- list the shared ELF/archive/object-facing types or helper seams later built-in work will need to include
- note missing or duplicated declaration boundaries that block compile-time integration of later binary-utils trees

Completion check:

- the repo has a concrete map of the current boundary, with named code surfaces and a bounded list of contract declarations to preserve or stage

## Step 2: Capture Representative Baseline Cases

Goal: lock a minimal but representative set of external-toolchain flows that later built-in assembler and linker work must match.

Primary targets:

- one tiny single-object case
- one relocation-bearing multi-object case

Actions:

- create or identify the smallest AArch64-emitted cases that exercise the current path
- preserve emitted assembly, produced object metadata, and linked executable expectations for each case
- ensure the cases cover labels, directives, symbol references, and at least the relocation shapes naturally exercised by the current backend subset

Completion check:

- at least two baseline cases exist and are inspectable end to end, with emitted assembly and object or executable expectations captured in repo-local artifacts

## Step 3: Document Assembly And Object Contract Details

Goal: turn the captured cases into an explicit compatibility contract for the binary-utils track.

Primary targets:

- assembly constructs in active use
- relocatable object expectations
- executable-side linker expectations

Actions:

- record the exact directives, label forms, symbol typing or visibility assumptions, numeric-label usage, and relocation-triggering patterns exercised by the baseline cases
- document the required object-file properties later assembler work must preserve: ELF class, section inventory, symbol-table essentials, relocation-table essentials
- document the executable-side expectations later linker work must preserve: entry symbol assumptions, startup ordering if relevant, archive-loading behavior if exercised
- make failure attribution explicit between backend emission, assembler parsing or encoding, and linker layout or resolution

Completion check:

- the binary-utils contract is clear enough that later assembler and linker ideas can cite one repo-local baseline instead of reverse-engineering expectations

## Step 4: Stage Shared Contract Surfaces

Goal: make later built-in binary-utils code able to depend on a bounded shared contract surface instead of inventing local declarations.

Primary targets:

- shared ELF or archive declarations
- object or linker-facing interfaces needed for compile integration

Actions:

- add or tighten the minimal shared declarations that later binary-utils trees must include
- keep those surfaces narrow and aligned with the boundary documented in Steps 1 through 3
- avoid introducing implementation-heavy binary-utils code in this plan

Completion check:

- later assembler or linker compile-integration work can include a bounded shared contract surface without ad hoc local duplicates

## Step 5: Lock Golden Validation

Goal: preserve the baseline as an acceptance target for the next binary-utils plans.

Actions:

- add focused tests or golden inspection fixtures for the representative cases
- validate that the baseline artifacts stay attributable and easy to compare
- record any deferred follow-on work in the source idea if it is adjacent but not required for this contract baseline

Completion check:

- the repo contains repeatable validation for the baseline cases, and future binary-utils work has a stable acceptance target

## Acceptance Checks

- Shared ELF/archive/object/linker-facing interfaces needed by later binary-utils work are declared and include-reachable.
- The current AArch64 external assembler and linker boundary is documented concretely.
- Representative external-toolchain behavior is captured in tests or fixtures.
- Later assembler and linker plans can cite this runbook and its source idea as the compatibility baseline.
