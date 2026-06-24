# Codegen Obj CLI And Test Integration Runbook

Status: Active
Source Idea: ideas/open/333_codegen_obj_cli_and_test_integration.md
Activated from: ideas/open/329_native_object_emission_umbrella.md
Depends on closed children:
- ideas/closed/330_native_object_model_and_emission_api.md
- ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md
- ideas/closed/332_aarch64_minimal_relocatable_elf_object_emission.md

## Purpose

Expose native object emission through the compiler CLI and test infrastructure
without weakening the existing assembly route or hiding unsupported target
combinations behind fallback behavior.

## Goal

Make `--codegen obj` a deliberate, testable output form for the supported
backend targets, with route-specific diagnostics, output-file behavior, and
CTest labels that let supervisors select object-only, asm-only, and dual-route
proof.

## Core Rule

The compiler's object route must write `.o` bytes from the target-owned native
object emitters and shared object writer. Do not implement `--codegen obj` by
printing `.s` and invoking or parsing an assembler, and do not remove or weaken
existing `--codegen asm` coverage.

## Read First

- `ideas/open/333_codegen_obj_cli_and_test_integration.md`
- `ideas/open/329_native_object_emission_umbrella.md`
- `ideas/closed/330_native_object_model_and_emission_api.md`
- `ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md`
- `ideas/closed/332_aarch64_minimal_relocatable_elf_object_emission.md`
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- compiler CLI option parsing and output-file handling under `src/`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp`

## Current Scope

- Add or wire `--codegen obj` as a user-visible output form for supported
  target/backend combinations.
- Route supported object output through RV64 and AArch64 native object
  emission, not through textual assembly.
- Report unsupported target/output combinations with stable diagnostics.
- Add CLI tests for object selection, output filenames, unsupported cases, and
  asm/object coexistence.
- Add backend harness support for object-route compile/link/runtime smoke
  tests beside existing asm-route tests.
- Keep CTest names and labels precise enough for object-only, asm-only, and
  dual-route proof selection.

## Non-Goals

- Do not redesign the shared object model or target object emitters.
- Do not add new RV64 or AArch64 instruction encoding or relocation support
  except for integration defects that block already-accepted child behavior.
- Do not make c-testsuite default to object output.
- Do not remove, rename away, or weaken existing asm-route tests.
- Do not build or depend on textual assembler support.
- Do not absorb broad backend semantic fixes discovered by object-route smoke
  tests; split those into focused follow-up ideas when they are distinct.

## Working Model

The CLI route should separate output form from target choice:

```text
--target <triple> --codegen asm -> existing target asm route -> .s
--target <triple> --codegen obj -> target object route -> .o
```

Supported object targets should call target-local object emission APIs that
produce shared object records and serialized ELF bytes. Unsupported targets or
unsupported object features should fail clearly rather than falling back to asm.

The test harness should preserve existing asm-route tests and add object-route
tests beside them:

```text
asm route remains green
object route has focused CLI and runtime smoke coverage
dual-route tests prove both routes remain selectable
```

## Execution Rules

- Start with discovery of the current CLI option and backend entrypoint shape;
  record the chosen seam and first proof subset in `todo.md`.
- Keep output-form naming, file suffix behavior, and diagnostics stable enough
  for CMake-scripted tests.
- Prefer narrow backend entrypoints for object bytes over widening string-based
  assembly emitters.
- Add route labels such as object, asm, dual-route, RV64, and AArch64 where
  they help supervisors choose subsets.
- Prove both accepted target object routes if both RV64 and AArch64 are wired.
- Preserve existing `.s` route coverage for touched code paths.
- Escalate to a new idea instead of widening this child if object-route smoke
  exposes unrelated frontend, BIR, prepared-BIR, or target semantic bugs.

## Step 1: Inspect CLI And Backend Output-Form Seams

Goal: identify the smallest integration surface that can add `--codegen obj`
without disturbing the existing `--codegen asm` route.

Primary targets:

- compiler CLI option parsing and output-path handling under `src/`
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- `src/backend/mir/riscv/codegen/object_emission.*`
- `src/backend/mir/aarch64/codegen/object_emission.*`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/`

Actions:

- Inspect how `--codegen asm` is parsed, dispatched, and written to disk.
- Find the public backend seam for returning native object bytes for RV64 and
  AArch64.
- Identify unsupported target/output combinations and the diagnostic site that
  should own them.
- Inspect existing backend CMake helpers for route tests, failure tests, and
  external runtime smoke tests.
- Record the chosen CLI/backend seam, first supported target subset, labels,
  and proof command in `todo.md`.

Completion check:

- `todo.md` names the object-route integration seam, first test subset, likely
  owned files, unsupported combinations, and proof command for Step 2.

## Step 2: Add Backend Object Output Entrypoints

Goal: provide backend-level object emission entrypoints that return bytes and
diagnostics without involving printed assembly.

Actions:

- Add or wire RV64 and AArch64 object entrypoints through the existing backend
  facade.
- Keep target object support explicit; unsupported targets should report stable
  errors.
- Preserve existing assembly-returning APIs for `--codegen asm`.
- Add focused backend or MIR tests that prove the entrypoints call native
  object emission and produce ELF bytes for the already-supported smoke subset.

Completion check:

- Supported target object entrypoints produce ELF object bytes through native
  object emitters, unsupported targets fail clearly, and nearby asm route tests
  still pass.

## Step 3: Wire `--codegen obj` Through The CLI

Goal: expose object output selection and output-file writing through the user
CLI while preserving asm behavior.

Actions:

- Extend codegen option parsing and dispatch so `--codegen obj` selects the
  backend object route.
- Write object bytes to the requested output path, with `.o` behavior covered
  by tests where the CLI owns suffix or default-output policy.
- Keep `--codegen asm` output text and diagnostics unchanged except where
  tests prove intended coexistence.
- Reject unsupported target/object combinations with stable diagnostics and no
  silent asm fallback.

Completion check:

- CLI tests prove accepted object output, unsupported object diagnostics,
  output-file behavior, and asm/object coexistence.

## Step 4: Add Object-Route Smoke Harness Support

Goal: let backend tests compile, link, and run object-route cases independently
from asm-route cases.

Actions:

- Add CMake test helpers or route options for object-output smoke cases.
- Add RV64 and AArch64 object-route compile/link/runtime smokes where local
  tools make the proof reliable.
- Preserve existing external asm-route smoke tests.
- Label object, asm, and dual-route tests so supervisors can filter them
  independently.

Completion check:

- Backend CTest entries can run object-route smoke tests without selecting the
  asm route, and dual-route proof shows both output forms remain available.

## Step 5: Validate Handoff To Object-Route Scan

Goal: finish this child with enough CLI and harness proof for the broader
object-route scan/default-readiness child to run deliberately.

Actions:

- Run a fresh build plus focused object-route CLI, backend object, and nearby
  asm-route tests.
- Record proof commands, result counts, labels, supported combinations, and
  remaining unsupported route features in `todo.md`.
- Do not switch c-testsuite defaults or default backend output form in this
  child.
- Prepare handoff notes for `ideas/open/334_object_route_scan_and_default_readiness.md`.

Completion check:

- `--codegen obj` is exposed for accepted supported targets, asm coverage
  remains meaningful, object and dual-route tests are selectable, and the
  supervisor can ask the plan owner to close child 333 or hand off to child
  334.
