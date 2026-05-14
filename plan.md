# BIR/MIR Abstract Slot Contract Runbook

Status: Active
Source Idea: ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md

## Purpose

Move the BIR/prepared-to-MIR boundary away from string-derived register identity and toward structured abstract placement slots.

## Goal

Make prepared allocation facts describe abstract banks, pools, slot indexes, widths, and spill slots, with AArch64 target MIR owning the mapping from those slots to physical register records.

## Core Rule

BIR and prepared allocation must not choose or require ISA spellings such as `x0`, `x20`, `d0`, `rdi`, `rax`, or BIR display names as placement identity. Target MIR maps abstract slots to target register records; printers map target records to final assembly text.

## Read First

- `ideas/open/223_bir_mir_abstract_slot_contract_reject_string_names.md`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/target_register_profile.hpp`
- `src/backend/prealloc/target_register_profile.cpp`
- `src/backend/prealloc/prepared_printer.cpp`
- `src/backend/mir/aarch64/ALLOCATION_CONTRACT.md`
- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`
- focused `tests/backend/backend_prepare_*` tests
- focused `tests/backend/backend_aarch64_prepared_*` tests

## Current Targets And Scope

- Prepared allocation records and helper APIs that currently publish `register_name`, `occupied_register_names`, source/destination register names, fixed/preferred/forbidden register names, or related spelling-derived identity.
- A structured prepared placement model that can represent register bank, slot pool, slot index, contiguous width, and spill placement without target spellings.
- AArch64 prepared-to-MIR conversion that consumes abstract slots and maps them to `aarch64::abi::RegisterReference` inside the AArch64 target layer.
- AArch64 MIR record fields that currently preserve register spelling vectors or `std::string_view` provenance as semantic allocation data.
- Focused tests proving argument, return, long-lived, caller-temp or scratch, and spill placements before target spelling is printed.

## Non-Goals

- Do not implement a full optimized register allocator.
- Do not change final assembly printer spelling except where tests need to keep existing output stable after structured mapping.
- Do not remove debug or display names when they are explicitly provenance-only.
- Do not enable unrelated AArch64 backend cases or broaden scalar feature coverage because this work touches adjacent code.
- Do not rewrite backend test-tree ownership from idea 220 in this plan.
- Do not keep renamed string fields as authoritative identity under new names.

## Working Model

Execution should move from contract discovery to compatibility-preserving structure:

1. inventory all string-authoritative placement paths and current tests;
2. introduce structured abstract slot types in prealloc;
3. migrate prepared allocation records and printers/tests to treat spellings as display or compatibility only;
4. update AArch64 mapping so physical register selection happens inside target MIR;
5. remove semantic string provenance from AArch64 MIR records and add reject-path tests;
6. run focused and broader validation before close.

Each implementation packet should preserve existing live behavior while reducing reliance on string identity.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Preserve source intent in the idea file; do not edit it for ordinary execution notes.
- Prefer semantic contract changes and reusable helpers over named-case or spelling-shape checks.
- For code-changing steps, run `cmake --build --preset default` or the repo-equivalent build command before narrow proof.
- Focus tests on abstract slot identity before target mapping and on AArch64 `RegisterReference` after target mapping.
- Treat tests that only rename string expectations as insufficient proof.
- Reject expectation downgrades, unsupported marking, and fixture-shaped shortcuts as route drift.

## Steps

### Step 1: Inventory String Placement Identity

Goal: establish the exact authoritative string paths and the narrow proof targets before code edits.

Primary targets:
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/`
- `tests/backend/backend_prepare_*`
- `tests/backend/backend_aarch64_prepared_*`

Actions:
- Search for register spelling fields such as `register_name`, `occupied_register_names`, source/destination register names, fixed/preferred/forbidden register names, and `std::string_view` register provenance.
- Classify each use as authoritative placement, display/provenance, compatibility, or target-layer spelling.
- Identify the focused tests that currently assert pre-target register strings and the tests that should assert abstract slot identity instead.
- Record the first implementation packet and delegated proof command in `todo.md`.

Completion check:
- `todo.md` records the authoritative string paths, display-only exceptions, affected tests, and the recommended first implementation packet.
- Search proof is recorded in `test_after.log` or the supervisor-delegated proof log if the supervisor requires one for inspection.

### Step 2: Add Structured Prepared Placement Types

Goal: introduce the abstract slot model without changing external target spelling behavior.

Primary targets:
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/prealloc/prealloc.cpp`
- `src/backend/prealloc/target_register_profile.hpp`
- `src/backend/prealloc/target_register_profile.cpp`
- focused prealloc tests

Actions:
- Add structured types for register bank, slot pool, slot index, contiguous width, and spill slot placement.
- Provide helpers for argument, return, long-lived, caller-temp, reserved scratch, special/forbidden, and spill placement as justified by current code.
- Keep any temporary spelling fields explicitly named or documented as legacy/display/compatibility while migration continues.
- Add focused tests that prove structured placement values independently of target spellings.

Completion check:
- Focused prealloc tests prove representative abstract slot construction, equality or inspection helpers, width handling, and spill placement.
- Build plus narrow test proof is recorded.

### Step 3: Migrate Prepared Allocation Records Away From Authoritative Strings

Goal: make prepared allocation facts carry structured placement as their semantic identity.

Primary targets:
- prepared allocation records in `src/backend/prealloc/`
- prepared printer display code
- focused `backend_prepare_*` tests

Actions:
- Replace authoritative uses of `register_name`, `occupied_register_names`, source/destination register names, fixed/preferred/forbidden register names, and similar fields with structured placement fields.
- Mark any temporary string fields as legacy/display/compatibility and prevent new semantic consumers from relying on them.
- Update prepared printer behavior so human-readable output remains display-only.
- Convert focused tests from string placement assertions to abstract slot assertions where the target layer has not yet run.

Completion check:
- Prepared allocation facts can describe representative argument, return, long-lived, scratch, forbidden, and spill placements without target register names.
- Tests fail if pre-target placement requires a target spelling as semantic identity.
- Build plus focused prepared-allocation proof is recorded.

### Step 4: Move AArch64 Physical Register Mapping Into Target MIR

Goal: make AArch64 consume abstract slots and choose target register records inside the target layer.

Primary targets:
- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- AArch64 prepared conversion and record tests

Actions:
- Add or update AArch64 mapping helpers from prepared abstract slots to `aarch64::abi::RegisterReference`.
- Ensure normal AArch64 lowering does not parse or forward prepared register spellings.
- Preserve final assembly behavior through target register records and the existing printer route.
- Add focused tests for argument, return, long-lived, caller-temp or scratch, and spill-related mapping.

Completion check:
- AArch64 prepared-to-MIR conversion uses abstract slot identity for normal lowering and maps to physical register records only inside AArch64 code.
- Focused AArch64 tests prove mapping behavior without requiring pre-target string identity.
- Build plus narrow AArch64 prepared conversion proof is recorded.

### Step 5: Remove Semantic String Provenance From MIR Records

Goal: prevent BIR value names, block labels, temporaries, or display text from becoming MIR semantic identity.

Primary targets:
- `src/backend/mir/aarch64/codegen/records.hpp`
- `src/backend/mir/aarch64/codegen/records.cpp`
- AArch64 record and machine-printer tests

Actions:
- Remove or demote MIR record fields that preserve prepared register spellings as semantic allocation data, such as vectors of register-name `std::string_view`.
- Introduce explicit provenance/debug structures only where diagnostics need display text.
- Audit label, symbol, branch, register, and machine-instruction identity paths for display-name leakage.
- Add tests that reject or fail to compile when normal MIR identity is derived from BIR display names instead of structured IDs or target records.

Completion check:
- AArch64 MIR records no longer carry prepared register spellings as semantic allocation data.
- Display/provenance names, if retained, are separate from register, label, symbol, and instruction identity.
- Build plus focused record/printer proof is recorded.

### Step 6: Final Validation And Close Readiness

Goal: verify idea 223 acceptance without drifting into unrelated backend feature work.

Primary targets:
- focused prealloc tests
- focused AArch64 prepared conversion and record tests
- relevant backend smoke or CTest subsets chosen by the supervisor
- `todo.md`

Actions:
- Re-run the focused tests that prove structured abstract placement and target-layer mapping.
- Run broader backend validation when shared prealloc records, target register profile code, or AArch64 conversion surfaces changed across multiple tests.
- Confirm no supported case was weakened, hidden, or marked unsupported.
- Confirm no new AArch64 feature coverage was enabled without its own markdown owner.
- Summarize remaining legacy/display compatibility fields, if any, in `todo.md` for close review.

Completion check:
- Source idea acceptance criteria are satisfied.
- Regression guard can be run by the plan owner before closure for the chosen close scope.
