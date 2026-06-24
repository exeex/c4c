# Object Route Scan And Default Readiness Runbook

Status: Active
Source Idea: ideas/open/334_object_route_scan_and_default_readiness.md
Resumed after: ideas/closed/336_target_object_emitter_scalar_scan_expansion.md

## Purpose

Resume object-route scan/default-readiness from the green scalar baseline
unblocked by child 336. The route now has selectable RV64 object-runtime scalar
cases and AArch64 object-byte scalar cases; this runbook decides how far the
object route can broaden and whether object output is ready to become the
default backend output form.

## Goal

Broaden object-route validation across backend and c-testsuite-adjacent
coverage, preserve asm-route evidence, triage remaining object-specific gaps,
and record a default-route recommendation with commands and result counts.

## Core Rule

Do not hide object-route failures by weakening expectations, adding
expected-failure scan labels for supported cases, or falling back to asm. Keep
asm coverage meaningful while object-route coverage expands.

## Read First

- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `ideas/closed/336_target_object_emitter_scalar_scan_expansion.md`
- `todo.md`
- `test_before.log`
- `test_after.log`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `tests/c/` scan helpers before changing c-testsuite defaults

## Current Baseline

The restored scalar scan proof is green:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Result: 28/28 tests passed, and the non-decreasing regression guard passed
against the matching `test_before.log`.

Restored selectors:

- RV64 object runtime: `backend_obj_runtime_rv64_return_zero`,
  `backend_obj_runtime_rv64_return_add`,
  `backend_obj_runtime_rv64_two_arg_helper`,
  `backend_obj_runtime_rv64_return_add_sub_chain`,
  `backend_obj_runtime_rv64_local_temp`.
- AArch64 object bytes: `backend_cli_aarch64_return_zero_writes_elf_obj`,
  `backend_cli_aarch64_return_add_writes_elf_obj`,
  `backend_cli_aarch64_return_add_sub_chain_writes_elf_obj`.

## Non-Goals

- Do not implement broad target object encoders inside this scan umbrella.
- Do not remove or weaken asm-route coverage.
- Do not make `.o` the default route without recorded scan evidence.
- Do not absorb x86 object output, object stdout, c-testsuite defaults, or
  object `--backend-bir-stage semantic` unless a later step explicitly proves
  readiness and updates the recommendation.
- Do not silently fold distinct backend semantic bugs into this umbrella; split
  focused follow-up ideas when ownership is target-specific or broad.

## Execution Rules

- Prefer adding object-route scan cases beside existing asm-route tests.
- Keep labels/selectors stable: object, asm, dual-route, runtime, qemu, rv64,
  aarch64, scan, unsupported where appropriate.
- For each expansion, record supported cases, unsupported boundaries, and the
  exact proof command/result count in `todo.md`.
- If a case fails because a target encoder lacks a focused capability, create
  or request a child idea rather than weakening scan expectations.
- Use the restored 28-test proof as the scalar baseline for regression
  comparison.

## Step 1: Resume Scan Baseline And Candidate Map

Goal: inspect the current restored object-route selectors and choose the next
safe scan expansion after child 336.

Actions:

- Confirm current RV64 object-runtime and AArch64 object-byte scalar selectors
  in `tests/backend/CMakeLists.txt`.
- Inspect backend and `tests/c/` helper surfaces for object-route expansion
  points without changing c-testsuite defaults.
- Name next RV64 runtime candidates and AArch64 object-byte candidates that
  avoid known unsupported globals/data and broad memory/pointer features.
- Record the Step 2 owned files and proof command in `todo.md`.

Completion check:

- `todo.md` names the next scan expansion seam, candidate cases, unsupported
  boundaries, and proof plan.

## Step 2: Expand Object Scan Selectors Conservatively

Goal: add the next small set of object-route scan cases that should pass on
existing target support, without changing defaults.

Actions:

- Add object-route CTest registrations beside existing asm-route coverage.
- Keep RV64 runtime object cases and AArch64 object-byte cases independently
  selectable.
- Do not add known-red expected-failure scan labels.
- If the first failure is a target-emitter capability gap, stop and record a
  focused follow-up idea instead of broadening this runbook.

Completion check:

- New object-route scan cases pass with a focused proof command, and existing
  asm-route selectors remain green.

## Step 3: Triage Remaining Unsupported Boundaries

Goal: sort object-route failures into default-readiness blockers, target-owned
  follow-ups, or intentional unsupported combinations.

Actions:

- Record failures by ownership: target encoder, object writer, relocation,
  CLI/output policy, linker/runtime harness, or broader backend semantic bug.
- Create focused follow-up ideas for distinct target-emitter or semantic bugs.
- Keep unsupported target/feature diagnostics stable and explicit.
- Do not change default output behavior.

Completion check:

- `todo.md` and any new focused ideas identify concrete blockers and ownership
  without weakening scan coverage.

## Step 4: Default-Readiness Recommendation

Goal: decide whether object output is ready to become the default backend route
or should remain a documented dual-route option.

Actions:

- Run the agreed scan/default-readiness proof subset.
- Compare object-route and asm-route coverage and failure categories.
- Record dates, commands, result counts, and rationale.
- Recommend either keeping dual-route mode or switching default behavior in a
  separate implementation idea.

Completion check:

- The source idea has a recorded default-readiness recommendation and clear
  follow-up ownership for unresolved object-route work.
