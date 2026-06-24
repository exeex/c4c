# Object Route Scan And Default Readiness Runbook

Status: Active
Source Idea: ideas/open/334_object_route_scan_and_default_readiness.md
Resumed after: ideas/closed/338_aarch64_object_emitter_local_frame_and_scalar_frontier.md

## Purpose

Broaden object-route validation across backend and c-testsuite coverage, then
decide whether native object output is ready to become the default backend
route or should remain an explicit dual-route option.

## Goal

Resume parent idea 334 from the 39/39 expanded object-route baseline and
continue scan/default-readiness work without weakening existing asm-route or
object-route contracts.

## Core Rule

Do not claim scan/default-readiness progress by downgrading unsupported cases,
adding expected-failure labels, hiding target-emitter gaps, or removing asm
coverage. Object-route cases must either pass through native object bytes or be
triaged to a concrete owner and split into focused follow-up work.

## Read First

- `ideas/open/334_object_route_scan_and_default_readiness.md`
- `todo.md`
- `tests/backend/CMakeLists.txt`
- `tests/backend/cmake/run_backend_codegen_object_case.cmake`
- `tests/backend/cmake/run_backend_rv64_object_runtime_case.cmake`
- `tests/c/external/c-testsuite/src/`
- `tests/backend/case/`

## Current Baseline

The current expanded object-route baseline is green at 39/39 tests after child
338 restored AArch64 object-byte c-testsuite `00011.c` and `00012.c`.

```bash
set -o pipefail; (cmake --build --preset default && ctest --test-dir build -R '^(backend_object_model_records|backend_riscv_object_emission|backend_aarch64_object_emission|backend_cli_.*obj|backend_obj_runtime_.*|backend_rv64_runtime_(return_zero|return_add|two_arg_helper|two_arg_local_arg|two_arg_both_local_arg|two_arg_second_local_arg|local_arg_call|return_add_sub_chain|local_temp)|backend_cli_aarch64_asm_external_return_(zero|add|add_sub_chain)_smoke|backend_codegen_route_riscv64_external_no_storage_main_emits_return_path)$' --output-on-failure) > test_after.log 2>&1
```

Known green object additions from prior focused children:

- RV64 object runtime scalar/local-call cases through `local_arg_call.c` and
  `local_temp.c`.
- RV64 object runtime c-testsuite `00001.c` and `00002.c`.
- AArch64 object-byte scalar cases through `two_arg_helper.c`.
- AArch64 object-byte c-testsuite `00001.c`, `00002.c`, `00011.c`, and
  `00012.c`.

## Non-Goals

- Do not implement new target object encoders inside the scan/default-readiness
  umbrella.
- Do not switch c-testsuite defaults or backend defaults until the scan and
  rationale are recorded.
- Do not add AArch64 runtime unless a separate reliable runtime packet is
  explicitly chosen.
- Do not remove or weaken asm-route tests.
- Do not implement x86 object output, object stdout, semantic-BIR object mode,
  broad globals/data, pointers, aggregates, byval, indirect calls, or broad
  branch/control-flow inside an untriaged scan packet.

## Execution Rules

- Add object-route scan cases only when they are expected to pass through
  native object bytes.
- Use distinct selectable labels for object, asm, runtime, qemu,
  c_testsuite, smoke, scan, and dual-route groups.
- Preserve helper behavior for existing asm-route and object-route tests.
- Dry-run uncertain candidates under `/tmp` before registering them.
- If the next useful candidates fail at target object emission, record the
  owner and split a focused child idea instead of adding expected failures.
- Keep proof commands and result counts in `todo.md`.

## Step 1: Resume 39-Test Baseline And Candidate Map

Goal: inspect the current scan surface after child 338 and choose the next
conservative parent-owned packet.

Actions:

- Confirm current object, asm, runtime, qemu, c_testsuite, smoke, scan, and
  dual-route labels plus representative test names.
- Inspect or dry-run the next RV64 runtime and AArch64 object-byte candidates
  under `/tmp`.
- Decide whether parent 334 can add more selectable scan cases directly or
  whether remaining failures require another focused target-emitter child.
- Record the Step 2 owned files, proof command, unsupported/default-policy
  boundaries, and candidate decision in `todo.md`.

Completion check:

- `todo.md` records the 39/39 baseline, current selector map, next candidate
  decision, and exact Step 2 proof command or split recommendation.

## Step 2: Add Or Split Next Object Scan Packet

Goal: either add the next passing selectable object scan cases or split a
focused child for target-owned gaps.

Actions:

- If green candidates are identified, register them beside existing helpers
  without changing defaults or asm behavior.
- If blockers are target-emitter capability gaps, create or request a focused
  child idea and park parent 334 with durable resume notes.
- Run the packet proof selected by the supervisor.
- Update `todo.md` with proof and next default-readiness or split handoff.

Completion check:

- Parent-owned scan additions are green, or lifecycle state is coherently
  split to a focused child before default-readiness work continues.

## Step 3: Default-Readiness Decision

Goal: decide whether native object output can become the default backend route
or should remain a documented dual-route option.

Actions:

- Review the accumulated object-route scan evidence and unresolved boundaries.
- Compare asm-route and object-route coverage that remains selected.
- Record the default-route recommendation with exact dates, commands, result
  counts, and rationale.
- Do not change defaults unless the evidence supports it and the supervisor
  delegates that implementation explicitly.

Completion check:

- `todo.md` records a defensible default-readiness recommendation and any
  required follow-up ideas for unresolved object-route gaps.
