# Static-Local Object-Data Contract Publication Runbook

Status: Active
Source Idea: ideas/open/511_static_local_object_data_contract_publication.md

## Purpose

Publish prepared object-data authority for function-scope static storage so
later RV64 consumers can use explicit producer facts instead of reconstructing
storage from access patterns.

## Goal

Teach the producer/prealloc object-data path to publish stable labels, object
extents, alignment, initialized bytes, zero-fill ranges, and relocation-ready
identity for static-local objects that already appear in prepared direct global
access facts.

## Core Rule

Publish static-local storage authority in prepared object-data records. RV64
must not infer static-local object identity, size, bytes, zero-fill, alignment,
or relocation facts from symbol spelling, access facts, source shape, or one
representative gcc_torture row.

## Read First

- ideas/open/511_static_local_object_data_contract_publication.md
- docs/rv64_gcc_torture_post_contract/infrastructure_bucket_evidence.md
- docs/rv64_gcc_torture_post_contract/global_stack_frame_infrastructure_review.md
- build/agent_state/424_step2_infrastructure_classification/930513-2/
- src/frontend/hir/hir_types.cpp
- src/backend/prealloc/object_data.hpp
- src/backend/prealloc/object_data.cpp
- src/backend/prealloc/prepared_contract_verifier.hpp
- src/backend/prealloc/prepared_contract_verifier.cpp
- tests/backend/bir/backend_prealloc_prepared_contract_verifier_test.cpp
- tests/backend/mir/backend_riscv_object_emission_test.cpp

## Current Targets

- Representative row: `src/930513-2.c`
- Failure bucket: `unsupported_global_data`
- Owning layer: producer object-data contract
- Known missing authority:
  - static-local object label for `__static_local_eq_0`
  - object size and alignment
  - initialized data and/or zero-fill extents
  - storage-object identity shared by access facts and object-data facts
- Current diagnostic shape:
  - `status=missing_object_label`
  - `object_size_bytes=0`
  - `emitted_byte_count=0`
  - `zero_fill_byte_count=0`

## Non-Goals

- Do not implement or broaden RV64 selected object-data emission in this plan.
- Do not infer static-local storage facts in RV64 from symbol names, source
  declarations, direct global access facts, or testcase shape.
- Do not broaden unrelated global-data access widths, GOT-required globals,
  thread-local storage, dynamic initialization, F128 data, stack parameter
  homes, or aggregate ABI handling.
- Do not change expectations, unsupported markers, allowlists, runtime
  comparison, scan accounting, or default test contracts.

## Working Model

- Static locals already become globals in the frontend with stable internal
  storage names, but the selected object-data contract currently fails before
  publishing enough object authority.
- Prepared direct global access facts are useful consumers of object identity,
  but access facts alone do not define storage-object extent, bytes,
  zero-fill, alignment, or relocation readiness.
- The producer must make selected object-data records coherent before RV64 can
  consume the static-local route without target-side reconstruction.
- Existing fail-closed diagnostics for missing labels, extents, bytes,
  zero-fill, relocations, and conflicting facts must remain precise.

## Execution Rules

- Keep implementation changes in producer/prealloc publication and contract
  verification surfaces needed to publish static-local object-data facts.
- Reuse existing selected object-data structures and verifier status names
  where possible; add helpers only when they clarify producer authority.
- Add focused producer/prepared coverage first, then use backend proof only to
  show the old missing-object-label blocker is gone or narrowed to a consumer
  gap.
- For code-changing steps, use a build proof, focused prepared/contract tests,
  representative `src/930513-2.c` evidence, and a relevant backend subset.

## Step 1: Rebuild Static-Local Object-Data Evidence

Goal: Reconfirm the representative failure and the current prepared facts
before editing code.

Actions:

- Inspect the idea 424 handoff docs and the
  `build/agent_state/424_step2_infrastructure_classification/930513-2/`
  artifacts.
- Run or reuse focused dumps for `src/930513-2.c` as needed to show the
  direct global access facts, selected object-data status, object size,
  emitted-byte count, zero-fill count, and current backend rejection.
- Identify whether current `HEAD` already has partial static-local object-data
  publication and where the selected object-data path loses the storage
  object.
- Record the evidence, current diagnostic, and suspected producer boundary in
  `todo.md`.

Completion check:

- `todo.md` identifies the current static-local storage evidence, the
  `missing_object_label` selected object-data facts, the producer path that
  drops or omits object authority, and any existing partial support to
  preserve.

## Step 2: Trace Static-Local Storage Identity Publication

Goal: Locate the producer path that should connect static-local globals to
prepared selected object-data records.

Actions:

- Trace static-local lowering from frontend/global creation into BIR and
  prepared object-data publication.
- Trace direct global access facts for the static-local object and compare
  their identity with selected object-data candidate records.
- Identify the minimal producer-side place to publish object label text,
  object size, alignment, initialized bytes, zero-fill ranges, and relocation
  readiness.
- Identify unsupported or ambiguous static-local shapes that must remain
  fail-closed, including missing storage identity, conflicting labels,
  unknown extents, unsupported initializers, and ambiguous relocation facts.
- Record the intended helper boundaries and unsupported variants in `todo.md`.

Completion check:

- `todo.md` names the static-local storage identity path, the selected
  object-data publication entry point, the helper or data-flow boundary to
  change, and the shapes that remain rejected.

## Step 3: Publish Static-Local Object Identity And Extents

Goal: Populate selected object-data records with authoritative static-local
object identity, size, and alignment.

Actions:

- Publish a stable object label and label text for function-scope static
  storage from producer-owned storage identity.
- Publish object size and alignment from prepared producer facts, not from
  RV64 access reconstruction.
- Ensure direct global access facts and selected object-data facts agree on
  the object identity RV64 will later consume.
- Preserve fail-closed diagnostics for missing or conflicting object labels,
  missing extents, and unsupported storage identities.
- Add or update focused contract/printer coverage for identity, size, and
  alignment publication.

Completion check:

- `cmake --build build --target c4cll` passes.
- Focused producer/prealloc tests prove static-local object labels, size, and
  alignment are published, while ambiguous or missing-authority cases still
  reject.
- `todo.md` records the final identity/extent publication boundary.

## Step 4: Publish Static-Local Data And Zero-Fill Ranges

Goal: Populate selected object-data records with initialized bytes,
zero-fill ranges, and relocation-ready identity when producer facts are
coherent.

Actions:

- Publish initialized byte ranges for static locals with supported constant
  initializers.
- Publish zero-fill byte counts or ranges for static locals that require BSS
  storage.
- Publish relocation facts only when the producer has explicit
  relocation-ready identity.
- Keep unsupported initializer semantics, missing bytes, missing zero-fill,
  conflicting ranges, and ambiguous relocations fail-closed with precise
  verifier diagnostics.
- Add or update focused contract coverage for initialized, zero-fill, and
  rejected static-local object-data shapes.

Completion check:

- Focused producer/prealloc tests prove initialized bytes, zero-fill ranges,
  and relocation-ready facts where supported.
- Rejected static-local shapes still report producer-contract failures instead
  of becoming target-consumable by accident.
- `todo.md` records emitted data/zero-fill facts and remaining unsupported
  variants.

## Step 5: Add Ordinary-C Static-Local Backend Coverage

Goal: Prove the repaired producer contract through ordinary-C coverage without
making RV64 infer missing facts.

Actions:

- Add or update focused ordinary-C backend cases for function-scope static
  storage that exercise the prepared object-data contract.
- Cover at least one initialized static-local object and one zero-fill or
  default-initialized static-local object when producer facts exist.
- Use the representative `src/930513-2.c` route only as one proof point, not as
  the only test shape.
- If RV64 consumer support is incomplete after producer publication, verify
  the remaining diagnostic names the consumer gap and no longer reports
  `missing_object_label`.
- Do not weaken the existing missing-authority expected-failure coverage; move
  it only if the old fixture now has coherent producer facts.

Completion check:

- New or updated focused backend tests pass.
- The representative route advances past the old `missing_object_label`
  selected object-data failure, or narrows to an explicit RV64 consumer gap.
- `git diff --check` passes for touched files.

## Step 6: Validate And Handoff

Goal: Prove the slice and prepare closure or a narrowed follow-up.

Actions:

- Run the build, focused producer/prealloc tests, focused representative proof,
  focused static-local backend coverage, and the supervisor-selected backend
  subset.
- Confirm RV64 did not synthesize labels, extents, initialized bytes,
  zero-fill, alignment, or relocations from access facts or source shape.
- Confirm no expectation, unsupported-marker, allowlist, runtime-comparison,
  or scan-accounting downgrade was used as progress.
- Record remaining non-goal failures, such as broad global access widths,
  GOT-required globals, thread-local storage, stack-passed parameter homes, or
  RV64 consumer gaps, in `todo.md`.

Completion check:

- Required validation passes.
- `todo.md` contains the final proof summary and a closure or follow-up
  recommendation.
