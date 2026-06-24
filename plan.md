# Object Route Scan And Default Readiness Runbook

Status: Active
Source Idea: ideas/open/334_object_route_scan_and_default_readiness.md
Activated from: ideas/open/329_native_object_emission_umbrella.md
Depends on closed children:
- ideas/closed/331_rv64_minimal_relocatable_elf_object_emission.md
- ideas/closed/332_aarch64_minimal_relocatable_elf_object_emission.md
- ideas/closed/333_codegen_obj_cli_and_test_integration.md

## Purpose

Broaden object-route validation after the narrow `--codegen obj` integration
work, identify target-specific gaps without hiding them behind expectation
churn, and decide whether object output is ready to become the default backend
route.

## Goal

Create selectable object-route scan coverage for supported RV64 and AArch64
subsets, compare it with existing asm-route proof, triage failures into focused
ownership, and record a default-route recommendation with commands, result
counts, and rationale.

## Core Rule

Do not claim default readiness by weakening expectations, deleting asm-route
coverage, or hiding object-route failures behind generic unsupported results.
Object-route scan failures must either be fixed in focused slices or moved into
new child ideas with concrete target, layer, and proof ownership.

## Read First

- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `ideas/closed/333_codegen_obj_cli_and_test_integration.md`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/`
- `tests/c/`
- `src/apps/c4cll.cpp`
- `src/codegen/llvm/llvm_codegen.hpp`
- `src/backend/backend.hpp`
- `src/backend/backend.cpp`
- target object-emission tests for RV64 and AArch64 under `tests/backend/mir/`

## Current Scope

- Add or extend selectable object-route scan labels and helpers beside existing
  asm-route coverage.
- Run and triage object-route scan results for supported RV64 and AArch64
  subsets.
- Preserve meaningful asm-route proof while object-route coverage grows.
- Record unsupported cases and distinct semantic/backend gaps as follow-up
  ideas instead of weakening broad expectations.
- Decide whether object output should become the default backend route or stay
  as an explicit dual-route option.

## Non-Goals

- Do not redesign the shared object model or target object encoders.
- Do not add the initial `--codegen obj` CLI wiring; child 333 already did
  that.
- Do not remove, rename away, or weaken existing asm-route coverage.
- Do not fix every discovered backend semantic bug inside this scan child.
- Do not build textual assembler support.
- Do not switch defaults before scan commands, result counts, and regression
  comparison are recorded.

## Working Model

Treat object output as a parallel backend route with its own selectable proof:

```text
asm route    -> existing .s tests and runtime/link smokes
object route -> --codegen obj byte tests, link/runtime smokes, scan labels
dual route   -> representative checks showing both routes remain available
```

The scan should distinguish failures by layer:

```text
CLI/output policy -> codegen facade -> backend object facade -> target object
writer/relocations -> linker/runtime
```

Failures that are broader semantic lowering problems should not be absorbed
into this umbrella unless a narrow fix is clearly required for scan mechanics.

## Execution Rules

- Start by inspecting current object/asm labels, helper scripts, and scan
  surfaces; record the first scan subset and proof plan in `todo.md`.
- Keep object-route scan helpers independent from asm-route helpers unless a
  shared wrapper preserves both behaviors explicitly.
- Prefer small representative scan subsets before broadening to c-testsuite or
  runtime matrices.
- Preserve before/after logs for scan expansions and include asm-route
  coexistence proof in acceptance checkpoints.
- Create focused follow-up ideas under `ideas/open/` for distinct object-route
  failures that are not fixed in this child; include reviewer reject signals in
  each new idea.
- Do not mark unsupported cases by weakening existing positive expectations
  without explicit supervisor approval.

## Step 1: Inspect Current Object Scan Surface

Goal: identify the smallest reliable object-route scan expansion point after
child 333.

Primary targets:

- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/`
- existing `backend_cli_.*obj`, `backend_obj_runtime_.*`,
  `backend_riscv_object_emission`, and `backend_aarch64_object_emission` tests
- existing asm-route runtime and external CLI smoke tests
- relevant c-testsuite helper surfaces under `tests/c/`

Actions:

- List current object, asm, runtime, qemu, and dual-route labels and test
  names.
- Inspect how c-testsuite and backend runtime helpers choose compiler output
  form.
- Identify the first RV64 and AArch64 scan candidates that can run without
  widening into unsupported globals/data or unrelated semantic bugs.
- Record in `todo.md` the chosen scan seam, first proof subset, likely owned
  files for Step 2, and unsupported combinations that should become follow-up
  ideas if encountered.

Completion check:

- `todo.md` names the object-route scan seam, first scan candidates, proof
  command, and known unsupported/default-policy boundaries.

## Step 2: Add Selectable Object Scan Helpers

Goal: make object-route scan cases selectable independently from asm-route
cases without changing default suite behavior.

Actions:

- Add or extend CMake helpers for object-route scan cases beside existing asm
  helpers.
- Preserve existing asm route labels and test names.
- Add labels that distinguish `object`, `asm`, `dual-route`, target, runtime,
  linkability, and scan scope.
- Start with the first reliable supported subset identified in Step 1.

Completion check:

- CTest can select the new object scan subset without selecting asm-only cases,
  and existing asm-route tests still run under their previous selectors.

## Step 3: Run And Triage Object Scan Results

Goal: execute the selected object-route scan and classify failures by concrete
  ownership rather than expectation churn.

Actions:

- Run the Step 2 object scan with matching before/after logs.
- Classify failures by target, object writer/relocation, CLI/output policy,
  linker/runtime, or semantic lowering layer.
- Fix only narrow scan-harness or integration defects that are within this
  child.
- For distinct backend semantic or target feature gaps, create focused
  follow-up ideas under `ideas/open/` with concrete acceptance and reviewer
  reject signals.

Completion check:

- Scan results are stable and diagnosable, and unresolved failures are either
  fixed or recorded as focused follow-up ideas.

## Step 4: Preserve Dual-Route And Runtime Proof

Goal: show object-route scan growth did not make asm-route proof meaningless.

Actions:

- Run representative asm-route and object-route tests together.
- Keep RV64 object runtime smoke and AArch64 object byte proof selected where
  relevant.
- Add a narrow dual-route proof if the scan helper changes shared selection or
  output-form wiring.
- Record commands, labels, and result counts in `todo.md`.

Completion check:

- Object-route and asm-route proof remain independently selectable, and the
  combined proof passes or has documented, owned follow-up gaps.

## Step 5: Default-Readiness Recommendation

Goal: decide whether object output should become the default backend route or
remain an explicit dual-route option.

Actions:

- Review scan coverage, unresolved follow-up ideas, unsupported combinations,
  and asm-route coexistence proof.
- Record the recommendation with concrete dates, commands, result counts, and
  rationale.
- If default switching is not ready, state the blocking feature gaps and the
  selectors that should remain canonical for object-route proof.
- If default switching is ready, prepare a narrow follow-up plan for the actual
  default change rather than making it silently in this scan step.

Completion check:

- `todo.md` and, if closing, the source idea record the recommendation and the
  evidence behind it. The supervisor can ask the plan owner to close, split, or
  activate the next focused follow-up.
