# AArch64 Object Emitter Local Frame And Scalar Frontier Runbook

Status: Active
Source Idea: ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md
Split from: ideas/open/334_object_route_scan_and_default_readiness.md

## Purpose

Repair the focused AArch64 target object-emitter gaps that currently block
balanced c-testsuite/backend object scan expansion in parent idea 334.

## Goal

Support native AArch64 object emission for selected no-global scalar
local-frame memory records and the selected scalar multiply frontier needed by
the next c-testsuite/backend object candidates.

## Core Rule

Do not make scan progress through testcase-name matching, asm fallback,
external assemblers, expected-failure labels, or weaker object/asm contracts.
Support selected AArch64 machine record shapes semantically and keep CLI/default
policy unchanged.

## Read First

- `ideas/open/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md`
- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `src/backend/mir/aarch64/codegen/object_emission.cpp`
- `src/backend/mir/aarch64/codegen/object_emission.hpp`
- `tests/backend/mir/backend_aarch64_object_emission_test.cpp`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `tests/c/external/c-testsuite/src/00003.c`
- `tests/c/external/c-testsuite/src/00011.c`
- `tests/c/external/c-testsuite/src/00012.c`
- `tests/backend/case/param_slot.c`

## Current Baseline

Parent idea 334 has a green 37/37 expanded object-route baseline after adding
c-testsuite object smokes for `00001.c` and `00002.c`.

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

The next AArch64 candidates currently reject in target object emission:

- c-testsuite `00003.c`, `00011.c`, and backend scalar local rewrite cases need
  selected fixed-frame/local-memory object support.
- c-testsuite `00012.c` needs selected scalar multiply object support.

## Non-Goals

- Do not add AArch64 runtime execution.
- Do not implement broad AArch64 branch/control-flow, globals/data, pointers,
  aggregates, byval, indirect calls, or broad memory lowering.
- Do not implement RV64, x86, object stdout, semantic-BIR object mode, or
  c-testsuite default-route changes.
- Do not weaken existing object/asm tests or add expected-failure scan labels.

## Execution Rules

- Inspect selected machine records first and record the smallest target-owned
  seams in `todo.md`.
- Keep local-frame memory and scalar multiply as separate packets unless the
  selected object-emitter shape proves they share one minimal implementation
  path.
- Add scan registrations only after the target object emitter can produce
  native bytes for the case.
- Stop and record a blocker if a candidate requires broad frame/memory/control
  flow support beyond this child.

## Step 1: Inspect AArch64 Local-Frame And Multiply Object Rejections

Goal: record exact selected machine record shapes and first implementation
proof plan.

Actions:

- Inspect AArch64 selected records or asm for c-testsuite `00003.c`, `00011.c`,
  `00012.c`, backend `param_slot.c`, and one representative
  `two_arg_*_rewrite.c` case.
- Name the object-emission dispatch functions and instruction families that
  reject local frame-memory and multiply records.
- Decide the first minimal implementation slice and proof command.
- Record Step 2 owned files, unsupported boundaries, and proof in `todo.md`.

Completion check:

- `todo.md` names the local-frame memory seam, multiply seam, first minimal
  implementation slice, and exact Step 2 proof command.

## Step 2: Add AArch64 Selected Local-Frame Memory Object Support

Goal: support the smallest selected fixed-frame local-memory object shapes
needed by the first accepted scalar no-global cases.

Actions:

- Extend AArch64 object emission for selected stack adjust, local store/load,
  and required save/restore forms only as needed by the chosen cases.
- Add focused AArch64 object-emitter unit tests for the new fragments.
- Restore object-byte scan tests for accepted cases such as c-testsuite
  `00003.c` and/or `00011.c`, and one backend scalar local case if supported.

Completion check:

- New object-byte scan cases pass through native AArch64 ELF bytes, and the
  existing 37-test baseline remains green in the focused proof subset.

## Step 3: Add AArch64 Selected Scalar Multiply Object Support

Goal: support the selected multiply family needed by c-testsuite `00012.c` if
it is not already handled by Step 2.

Actions:

- Extend AArch64 object emission for the selected `mul` machine record shape.
- Add focused object-emitter encoding coverage.
- Restore `00012.c` as an AArch64 object-byte scan case if the route is
  otherwise supported.

Completion check:

- c-testsuite `00012.c` passes as native AArch64 object-byte output, and
  existing AArch64 scalar object cases remain green.

## Step 4: Validate Expanded AArch64 Object Frontier

Goal: prove the child’s accepted scan additions and hand back to parent idea
334.

Actions:

- Run the focused proof including AArch64 object-emitter tests, AArch64 object
  scan cases, existing RV64 object runtime baseline selectors, and asm-route
  smokes.
- Record commands, result counts, remaining unsupported target boundaries, and
  parent handoff in `todo.md`.

Completion check:

- `todo.md` records a green expanded baseline and explicitly says whether idea
  334 can resume default-readiness or needs another focused child.
