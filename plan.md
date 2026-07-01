# RV64 Selected Object-Data Emission Runbook

Status: Active
Source Idea: ideas/open/510_rv64_selected_object_data_emission.md

## Purpose

Repair the RV64 object/global-data consumer path for coherent prepared
selected object-data contracts.

## Goal

Teach RV64 object emission to consume prepared object labels, object extents,
initialized bytes, zero-fill ranges, and relocations from selected object-data
records without reconstructing storage from source shape, testcase identity, or
raw global symbols.

## Core Rule

Consume prepared selected object-data authority directly. If the prepared
contract is missing labels, extents, byte ranges, relocation facts, alignment,
or coherence, keep the route fail-closed with a diagnostic that names the
unsupported fact family.

## Read First

- ideas/open/510_rv64_selected_object_data_emission.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
- build/agent_state/424_step2_infrastructure_classification/20000412-1/
- src/backend/prealloc/object_data.hpp
- src/backend/prealloc/object_data.cpp
- src/backend/prealloc/prepared_contract_verifier.hpp
- src/backend/prealloc/prepared_contract_verifier.cpp
- src/backend/mir/riscv/codegen/object_emission.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Representative row: `src/20000412-1.c`
- Failure bucket: `unsupported_global_data`
- Owning layer: RV64 object/global-data emission
- Known prepared facts:
  - selected object-data status is `unsupported_but_coherent`
  - `object_label_id=2`
  - `object_size_bytes=1656`
  - `emitted_byte_count=0`
  - `zero_fill_byte_count=0`
- Nearby producer gaps:
  - `src/930513-2.c` remains `missing_object_label`
  - stack-passed parameter-home rows remain producer ABI/prealloc work

## Non-Goals

- Do not repair producer publication for missing static-local object labels,
  object extents, initialized bytes, zero-fill ranges, or parameter homes.
- Do not infer object labels, extents, data bytes, zero-fill, relocations, or
  section placement in RV64 from source names, C declarations, raw globals, or
  one gcc_torture row.
- Do not broaden unrelated global-memory access widths, GOT-required globals,
  thread-local storage, F128 data, or unsupported address modes.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, scan accounting, or default test contracts.

## Working Model

- The prepared object-data contract classifier separates producer-missing,
  producer-incoherent, coherent, and target-unsupported-but-coherent records.
- RV64 may consume a selected object-data record only when the prepared facts
  give explicit object identity, extent, section/data authority, and any
  needed relocation authority.
- Coherent selected object-data support should emit object symbols into the
  proper object section, append initialized bytes or reserve zero-fill bytes,
  and attach relocations from prepared records.
- Producer gaps such as `missing_object_label` are blockers for later producer
  ideas and must not be repaired in RV64 by reconstructing storage.

## Execution Rules

- Keep implementation changes in RV64 object emission and nearby prepared
  object-data helpers needed to consume existing prepared records.
- Prefer small validation/emission helpers only when they make selected
  object-data admission, section choice, byte emission, zero-fill, or
  relocation handling clearer.
- Add focused object-emission and ordinary-C backend coverage that proves
  initialized data, zero-fill, and relocation behavior through object output or
  runtime execution, not scan-bucket accounting.
- For code-changing steps, use a build proof, focused representative proof,
  focused object-data coverage, and an appropriate `^backend_` subset.

## Step 1: Rebuild Selected Object-Data Evidence

Goal: Reconfirm the representative failure and the prepared selected
object-data contract before editing code.

Actions:

- Inspect the idea 424 handoff docs and the
  `build/agent_state/424_step2_infrastructure_classification/20000412-1/`
  artifacts.
- Run or reuse focused dumps for `src/20000412-1.c` as needed to show the
  prepared object-data facts and the current RV64 object-route rejection.
- Record the object label, object extent, emitted-byte count, zero-fill count,
  relocation state, section authority, and rejection point in `todo.md`.
- Check whether current `HEAD` already has partial selected object-data
  support and identify the exact remaining unsupported shape before changing
  code.

Completion check:

- `todo.md` identifies the prepared selected object-data facts, the current
  unsupported diagnostic, the code path that rejects the coherent contract, and
  any existing partial support that must be preserved.

## Step 2: Trace RV64 Object-Data Consumption

Goal: Locate the minimal RV64 consumer path that should validate and emit
coherent selected object-data records.

Actions:

- Trace prepared object-data publication from
  `prepare::populate_prepared_object_data_plans` through contract
  verification into RV64 object emission.
- Identify existing section creation, symbol definition, byte append,
  zero-fill reservation, and relocation helpers that should be reused.
- Identify unsupported variants that must continue to fail closed, including
  missing object labels, missing extents, ambiguous relocations, unsupported
  marker-only records, thread-local storage, GOT-required globals, and
  producer-incoherent records.
- Record intended helper boundaries and blast radius in `todo.md`.

Completion check:

- `todo.md` names the prepared fact path, the RV64 consumer entry point, the
  helpers to reuse or add, and the unsupported variants that remain rejected.

## Step 3: Implement Contract Admission And Fail-Closed Guards

Goal: Admit only selected object-data records with enough prepared authority
for RV64 emission and keep producer gaps fail-closed.

Actions:

- Add or tighten RV64 validation around prepared selected object-data contract
  status and required fields before section emission.
- Distinguish coherent target-consumable records from
  `TargetUnsupportedButCoherent` records that still lack bytes, zero-fill, or
  relocation facts needed for emission.
- Preserve precise diagnostics for missing labels, missing object byte ranges,
  missing emitted bytes, missing zero-fill, missing relocations, conflicting
  facts, and invalid pre-prepared initializer semantics.
- Add or update focused object-emission tests for admitted and rejected
  contract shapes.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused object-emission tests prove admitted selected object-data contracts
  build and producer-missing or incoherent contracts still reject.
- `todo.md` records proof commands and the final admission boundary.

## Step 4: Emit Prepared Object Data

Goal: Emit initialized data, zero-fill, object symbols, and relocations from
prepared selected object-data records.

Actions:

- Emit object symbols using prepared object-label text and object extents.
- Choose `.data`, `.rodata`, or `.bss` from prepared section authority rather
  than source spelling or testcase shape.
- Append prepared initialized bytes for data sections and reserve prepared
  zero-fill extents for BSS.
- Attach relocations only from prepared relocation-ready identity and keep
  ambiguous or missing relocation facts fail-closed.
- Ensure symbol size, section offset, alignment, and relocation records are
  derived from prepared object-data records.

Completion check:

- Focused object-emission tests prove initialized bytes, zero-fill reservation,
  object symbol extent, and relocation record behavior.
- Focused representative proof advances past the old selected object-data
  rejection when the prepared facts are sufficient.
- `todo.md` records emitted section/symbol/relocation shapes and remaining
  unsupported variants.

## Step 5: Add Ordinary-C Backend Coverage

Goal: Prove the repaired route with ordinary-C coverage that is not tied to one
gcc_torture row.

Actions:

- Add or update focused ordinary-C backend cases that exercise selected
  object-data emission through prepared facts.
- Cover at least one initialized object-data path and one zero-fill or
  relocation path when the prepared producer facts exist.
- Include a fail-closed coverage point for a missing-object-label or
  missing-authority shape, without weakening unsupported contracts.
- Do not rely on `src/20000412-1.c`, object label `2`, object size `1656`, or
  one exact initializer layout as the only proof.

Completion check:

- New or updated focused backend tests pass.
- The representative route remains advanced past the repaired selected
  object-data failure when in scope.
- `git diff --check` passes for touched files.

## Step 6: Validate And Handoff

Goal: Prove the slice and prepare the source idea for closure or a narrowed
follow-up if producer facts are still missing.

Actions:

- Run the build, focused representative proof, focused object-data coverage,
  and `ctest --test-dir build -j --output-on-failure -R "^backend_"`.
- Confirm `src/930513-2.c` and other producer-missing rows remain fail-closed
  unless a separate producer idea publishes the missing facts.
- Confirm no expectation, unsupported-marker, allowlist, runtime-comparison, or
  scan-accounting downgrade was used as progress.
- Record any remaining non-goal failures, such as static-local object-data
  publication, unsupported global-memory widths, GOT-required globals,
  thread-local storage, or parameter-home publication, in `todo.md`.

Completion check:

- Backend validation passes.
- `todo.md` contains the final proof summary and closure or follow-up
  recommendation.
