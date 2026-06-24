# Object Route Scan And Default Readiness Runbook

Status: Active
Source Idea: ideas/open/334_object_route_scan_and_default_readiness.md
Resumed after: ideas/closed/337_target_object_emitter_local_call_and_regreg_scalar_expansion.md

## Purpose

Broaden native object-route validation from the restored scalar/local-call
baseline toward default-readiness evidence, while keeping asm-route coverage
meaningful and preserving clear diagnostics for unsupported object cases.

## Goal

Add and triage selectable object-route scan coverage for supported RV64 and
AArch64 subsets, then record whether object output is ready to become the
default backend route or should remain a documented dual-route option.

## Core Rule

Do not claim scan progress by weakening expectations, adding expected-failure
labels for known-red cases, deleting asm-route coverage, or hiding object-route
failures behind generic toolchain errors. Fix focused target/backend gaps in
separate child ideas when they are distinct capability work.

## Read First

- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `tests/c/` helper surfaces if c-testsuite object selection is considered

## Current Baseline

The expanded object-route scan baseline after child 337 is green:

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Result: 33/33 tests passed, with non-decreasing regression guard against the
rolled baseline.

Restored selectors include:

- RV64 object runtime scalar/local-call cases under `backend_obj_runtime_.*`
- AArch64 scalar object-byte cases under `backend_cli_aarch64_.*_writes_elf_obj`
- Existing asm-route RV64 runtime and AArch64 external-return smokes in the
  same proof subset

## Scope

- Add selectable object-route scan coverage beside existing asm-route tests for
  cases that the target object emitters can support natively.
- Triage object-route failures to encoder, object writer, relocation, CLI,
  linker, or runtime layers.
- Preserve route labels that let CTest select object, asm, dual-route, runtime,
  qemu, RV64, and AArch64 coverage independently.
- Record any distinct target-emitter or default-policy blockers as follow-up
  ideas instead of silently expanding this umbrella.
- Decide default-readiness only after recorded scan commands, result counts,
  and rationale exist.

## Non-Goals

- Do not implement shared object model rewrites or broad target object encoders
  inside this scan umbrella.
- Do not add initial `--codegen obj` CLI wiring; it already exists.
- Do not remove or weaken asm-route coverage.
- Do not force object stdout, x86 object output, semantic-BIR object mode, or
  c-testsuite object defaults without explicit scan evidence and a plan step.
- Do not absorb globals/data, arrays, pointers, aggregates, broad memory/control
  flow, broad AArch64 frame/local-memory expansion, or AArch64 runtime fixes
  into a scan-label packet.

## Execution Rules

- Start each scan expansion from the current green baseline and keep proof
  subsets selectable by label or stable regex.
- Add only cases that are expected to pass through native object bytes; do not
  add known-red cases as expected failures.
- When a candidate fails because of target object-emitter capability, stop and
  record or split a focused child idea.
- Keep backend runtime helpers and c-testsuite helper defaults behaviorally
  unchanged unless the step explicitly owns them.
- Use `todo.md` for packet evidence and handoff notes; edit this runbook only
  for lifecycle-level route changes.

## Step 1: Resume Expanded Scan Baseline And Candidate Map

Goal: inspect the object-route scan surface after child 337 and choose the next
conservative scan/default-readiness packet.

Actions:

- Confirm current object/asm/runtime/qemu/dual-route labels and representative
  test names from `tests/backend/CMakeLists.txt` and backend CMake helpers.
- Identify the next safe RV64 runtime and AArch64 object-byte scan candidates,
  or name the exact target/default-policy blocker if none should be added yet.
- Check whether any c-testsuite object-route selection can be introduced
  without changing defaults or masking unsupported cases.
- Record Step 2 owned files and proof command in `todo.md`.

Completion check:

- `todo.md` confirms the restored 33/33 baseline, names next candidates or
  blockers, preserves unsupported/default-policy boundaries, and gives an
  exact Step 2 proof command.

## Step 2: Add Next Selectable Object Scan Cases

Goal: add the next conservative object-route scan coverage that is expected to
pass from the current baseline.

Actions:

- Add object-route CTest registrations or helper extensions beside existing
  asm-route coverage for the selected candidates.
- Keep object, asm, dual-route, runtime, qemu, RV64, AArch64, scalar, and scan
  labels useful for independent selection.
- Do not change c-testsuite defaults unless Step 1 explicitly proves that is
  the intended target for this step.
- Run the focused proof command recorded by Step 1 and update `todo.md`.

Completion check:

- New object-route scan cases are green, existing asm-route selectors remain
  selected and green, and `todo.md` records result counts and remaining
  boundaries.

## Step 3: Triage Remaining Object-Route Gaps

Goal: classify remaining unsupported object-route scan candidates without
weakening expectations.

Actions:

- Dry-run or inspect representative unsupported candidates from the scan map.
- Classify failures by owner: target encoder, object writer, relocation, CLI,
  linker/toolchain, runtime, or policy/default readiness.
- Create or request focused child ideas for distinct capability gaps that
  should precede more scan expansion.
- Keep known unsupported cases out of green selectors until repaired.

Completion check:

- `todo.md` records the triage map, follow-up idea needs, and whether the
  active plan should continue, split, or pause for a child.

## Step 4: Default-Readiness Recommendation

Goal: decide whether object output should become the default backend route or
remain an explicit dual-route option.

Actions:

- Compare object-route and asm-route scan evidence for supported RV64 and
  AArch64 subsets.
- Record commands, dates, result counts, unsupported boundaries, and rationale.
- If default switching is not justified, document the remaining blockers and
  keep object output opt-in.
- If default switching is justified, define a separate implementation plan for
  the default-route policy change.

Completion check:

- `todo.md` and, if closing, the source idea record the recommendation and
  evidence without changing defaults implicitly.
