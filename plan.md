# Route 4 Publication-Source Semantic Reader Runbook

Status: Active
Source Idea: ideas/open/209_route4_publication_source_semantic_reader.md

## Purpose

Activate one narrow Route 4 follow-up that moves exactly one publication-source
semantic reader to route/prepared agreement while keeping prepared publication
mechanics and output policy authoritative.

## Goal

Select and migrate one named publication-source reader so valid Route 4
publication identity can supply the same source identity as prepared lookup
state, with prepared fallback preserved for every invalid or ambiguous case.

## Core Rule

Route 4 may identify the selected publication source only. It must not take
ownership of publication mechanics, move/home/storage policy, immediate payload
spelling, wrapper behavior, emitted output, debug text, prepared printer rows,
or public fallback removal.

## Read First

- `ideas/open/209_route4_publication_source_semantic_reader.md`
- `docs/bir_prealloc_fusion/phase_d2_retained_surface_consumer_switch_analysis.md`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

## Current Targets

- One publication-source semantic reader adjacent to existing Route 4
  current-block publication identity support.
- Route 4 publication availability/reference validation and prepared
  publication-source fallback.
- AArch64 call-boundary or publication-source reader tests that can prove
  route/prepared agreement without changing emitted strings.

## Non-Goals

- Do not migrate wrapper families, CLI dumps, prepared printer rows, x86/riscv
  publication wrappers, or diagnostics.
- Do not delete prepared APIs, public fallbacks, aggregate lookup fields, or
  prepared publication helpers.
- Do not move publication mechanics, edge-copy emission, move/home/storage
  policy, stack-source extension, immediate spelling, register-view conversion,
  or final output policy into Route 4.
- Do not weaken supported-path tests, refresh baselines, or change expected
  strings as proof.

## Working Model

- Prepared publication lookup state remains the compatibility and fallback
  authority.
- Route 4 evidence is acceptable only when it agrees with the selected prepared
  source identity for the named reader.
- Missing, duplicate, ambiguous, stale, wrong-reference, or mismatched Route 4
  facts must fail closed to existing prepared behavior.
- Wrapper and printer/debug output should remain byte-stable throughout the
  slice.

## Execution Rules

- Keep each implementation packet tied to one named reader.
- Prefer helper extraction only when it makes route/prepared agreement and
  fail-closed behavior clearer for that reader.
- Add or extend tests before claiming capability progress when a negative case
  is not already covered.
- Treat expectation rewrites, helper renames, unsupported downgrades, and
  baseline refreshes as non-progress.
- For code-changing packets, run a fresh build or compile proof plus the narrow
  Route 4/AArch64 publication-source test subset selected by the supervisor.
- Escalate to broader backend validation if public signatures, wrapper
  behavior, prepared aggregate exposure, or expected strings are touched.

## Steps

### Step 1: Name the Reader and Baseline the Existing Route 4 Boundary

Goal: identify the exact publication-source semantic reader to migrate and
confirm current Route 4/prepared fallback coverage.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Inspect existing Route 4 current-block publication identity helpers and the
  call-boundary publication source reader paths.
- Choose one named reader that is still a publication-source semantic consumer,
  not a printer/debug row or wrapper surface.
- Record the chosen reader and the exact proof command in `todo.md` when
  execution begins.
- Confirm existing positive, absent, stale/wrong-reference, duplicate or
  ambiguity, mismatch, and prepared-fallback coverage.

Completion check:
- `todo.md` names the selected reader, the narrow proof command, and any
  missing proof cases without changing implementation files.

### Step 2: Add Route/Prepared Agreement for the Named Reader

Goal: let the selected reader use Route 4 evidence only when it agrees with the
prepared publication source identity.

Primary targets:
- `src/backend/mir/aarch64/codegen/calls.cpp`
- Route 4 publication availability/index helpers already used by nearby
  reader code

Actions:
- Add or reuse a narrow route/prepared agreement helper for the named reader.
- Require matching value name, value id when available, value type, block or
  relationship identity, source producer instruction, and before-instruction
  boundary.
- Preserve prepared fallback for missing Route 4 facts and for every failed
  agreement check.
- Keep prepared lookup construction, publication mechanics, output policy, and
  target wrapper behavior unchanged.

Completion check:
- The named reader returns the same source identity under route/prepared
  agreement and preserves old prepared behavior for absent or invalid route
  data.

### Step 3: Prove Negative and Fallback Cases

Goal: make the fail-closed contract observable for the migrated reader.

Primary targets:
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:
- Cover wrong-reference or stale Route 4 data.
- Cover duplicate, ambiguous, or conflicting Route 4 publication rows when the
  selected reader can observe them.
- Cover route/prepared disagreement and prepared fallback.
- Include a route-only/no-prepared-fallback case only when it reflects the
  reader's existing fail-closed semantics.

Completion check:
- Narrow tests prove positive, absent, invalid, duplicate/conflict when
  applicable, mismatch, and prepared fallback behavior for the selected reader.

### Step 4: Preserve Output and Wrapper Stability

Goal: prove the migration did not alter byte-stable output surfaces.

Primary targets:
- Existing AArch64 expected-string checks for the selected reader
- x86/riscv publication wrapper tests or prepared debug/printer tests selected
  by the supervisor

Actions:
- Run the selected no-output-change proof for AArch64 output touched by the
  reader.
- Run wrapper/debug stability checks if the implementation touched any shared
  publication lookup or helper used outside the selected reader.
- Do not update expected strings unless a separate approved semantic change
  requires it.

Completion check:
- Proof logs show the named reader changed only its internal source identity
  authority and no wrapper, printer/debug, or emitted strings changed.

### Step 5: Acceptance Review

Goal: prepare the slice for supervisor review without expanding scope.

Actions:
- Check the final diff against the source idea reject signals.
- Verify no broad publication migration, wrapper migration, prepared API
  deletion, or expected-string weakening slipped in.
- Keep routine progress and proof notes in `todo.md`; do not edit the source
  idea unless durable intent changed.

Completion check:
- The slice satisfies the source idea acceptance criteria and is ready for
  supervisor-side validation and commit.
