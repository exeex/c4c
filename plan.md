# RV64 Helper-Free Variadic Entry Contract Runbook

Status: Active
Source Idea: ideas/open/367_rv64_helper_free_variadic_entry_contract.md

## Purpose

Define the RV64 object-route contract for helper-free variadic function entries
without bypassing the explicit variadic runtime gate or overfitting
`src/20030914-2.c`.

## Goal

Make RV64 helper-free variadic entry admission either consume a real prepared
runtime contract or fail with a precise structured diagnostic that explains the
missing contract.

## Core Rule

Progress must be based on prepared variadic entry/runtime facts and focused
backend coverage, not testcase names, source-shape matching, skip broadening,
or expectation downgrades.

## Read First

- `ideas/open/367_rv64_helper_free_variadic_entry_contract.md`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

## Current Targets

- RV64 prepared function admission for variadic entries that do not require a
  backend-emitted `va_start` helper path.
- Structured unsupported diagnostics for missing helper-free variadic runtime
  contracts.
- Focused backend tests that preserve existing RV64 variadic helper and
  parameter-home coverage.
- Representative rerun for `src/20030914-2.c`.

## Non-Goals

- Do not implement scalar or aggregate `va_arg`, `va_copy`, or new `va_start`
  helper lowering beyond facts already supplied by prior variadic ideas.
- Do not implement byval aggregate parameter-home lowering in prepared stack
  slots; idea 363 already documented that separate boundary.
- Do not move shared LIR/BIR vararg semantics into RV64 object emission.
- Do not weaken function admission so unsupported variadic entries silently
  reach later emission paths.
- Do not claim scan progress from expectation rewrites, skips, or allowlist
  filtering.

## Working Model

`src/20030914-2.c` currently stops at:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```

The plan should first identify what prepared facts distinguish a helper-free
variadic entry from an entry requiring runtime helper materialization. Only
after that audit should implementation choose between admitting a supported
contract or preserving rejection with a more precise diagnostic.

## Execution Rules

- Keep object emission consuming explicit prepared facts; do not reconstruct
  missing variadic semantics in the target emitter.
- Preserve the existing prepared-GPR and overflow-base contracts from the
  recent RV64 variadic work.
- Keep diagnostics structured and specific enough that representative reruns
  can distinguish helper-free entry admission from parameter-home and `va_arg`
  boundaries.
- Update `todo.md` after each executor packet with changed files, proof
  results, and the next boundary.
- Use `test_after.log` for executor proof unless the supervisor delegates a
  different artifact.

## Steps

### Step 1: Audit Helper-Free Variadic Entry Facts

Goal: Determine which prepared facts are available when RV64 rejects a
helper-free variadic function entry.

Primary targets:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- prepared dumps/logs for `src/20030914-2.c`

Actions:

- Locate the function-admission gate that emits the current
  `unsupported_function_admission` diagnostic.
- Identify how the prepared module represents a variadic entry with no emitted
  `va_start` helper requirement.
- Compare the available facts against prior RV64 variadic helper contracts:
  helper homes, overflow-area state, destination address state, and parameter
  homes.
- Decide whether the current data supports a positive admission contract or
  only a refined unsupported contract.

Completion check:

- `todo.md` records the exact boundary, owned producer/consumer surfaces, and
  a focused Step 2 implementation packet.
- Audit-only validation runs `git diff --check -- todo.md`.

### Step 2: Define and Cover the Admission Contract

Goal: Add focused backend coverage for the chosen helper-free variadic entry
contract.

Primary targets:

- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp`

Actions:

- Add or adjust focused tests that prove the selected admission behavior.
- If the contract remains unsupported, assert the exact structured diagnostic
  and keep it distinct from parameter-home, `va_start`, and `va_arg`
  diagnostics.
- If the contract is admissible, assert the prepared facts that make it safe
  and preserve existing unsupported tests for entries missing those facts.

Completion check:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure
```

### Step 3: Implement the RV64 Admission Behavior

Goal: Make RV64 object emission follow the tested helper-free variadic entry
contract.

Primary target:

- `src/backend/mir/riscv/codegen/object_emission.cpp`

Actions:

- Consume the prepared facts identified by Step 1.
- Keep unsupported entries behind a structured diagnostic instead of silently
  falling through to invalid emission.
- Avoid changing unrelated RV64 variadic helper lowering, parameter-home
  lowering, or allowlists.

Completion check:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure
```

### Step 4: Rerun the Representative Boundary

Goal: Verify `src/20030914-2.c` advances honestly or documents the next
structured boundary.

Actions:

- Rerun only `src/20030914-2.c` through the RV64 gcc torture backend runner.
- Preserve the resulting diagnostic in `todo.md`.
- If the case reaches the byval stack-slot parameter-home boundary from idea
  363, treat that as expected out-of-scope progress.
- If it reaches a different variadic or `va_arg` boundary, document whether it
  belongs to this idea or a separate follow-up.

Completion check:

```sh
tmp=$(mktemp); printf 'src/20030914-2.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh; rc=$?; rm -f "$tmp"; exit $rc
```

### Step 5: Close Readiness Review

Goal: Decide whether idea 367 is complete or needs a follow-up split.

Actions:

- Confirm focused backend tests remain green.
- Confirm the representative rerun no longer stops at the old helper-free
  variadic admission diagnostic unless the diagnostic was intentionally refined
  and accepted as the final contract.
- Confirm any new boundary is documented in `todo.md` and routed to the
  correct existing or new idea.

Completion check:

- Plan owner can close the idea with focused regression proof, or can create a
  separate open idea for any out-of-scope boundary.
