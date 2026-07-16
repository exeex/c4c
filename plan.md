# LIR Canonical Module-Owned Aggregate Ref/Store Convergence Runbook

Status: Active
Source Idea: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Resumed from: closed 852 HIR aggregate-ref binding handoff at 838 Step 2

## Purpose

Implement the first nominal-family owner: stable aggregate identity from HIR
occurrences through a module-owned LIR aggregate store, without absorbing the
parked 836 route or adjacent nominal-family migrations.

## Goal

Make aggregate-bearing occurrences use a stable canonical HIR aggregate
reference and intern it exactly once into the owning LIR module's aggregate
store.

## Core Rule

Aggregate identity comes from canonical HIR facts, never tags, parser pointers,
rendered text, `record_def`, `Node*`, runtime strings, or reconstructed lookup
keys. Preserve the three 836 failure groups as distinct evidence; do not
complete 836 or change its 831 Step 4 return.

## Read First

- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- `ideas/closed/852_hir_canonical_semantic_aggregate_ref_binding.md`
- `docs/lir_nominal_type_family_architecture/current_lir_type_ref_responsibility_matrix.md`
- `docs/lir_nominal_type_family_architecture/nominal_family_boundary_decisions.md`
- `docs/lir_nominal_type_family_architecture/dependency_ordering.md`
- `docs/lir_nominal_type_family_architecture/closure_trace.md`
- `ideas/open/836_lir_remaining_aggregate_owner_rejection_decomposition_blocker.md`
- `ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md`

## Current Targets

- Step 1 is accepted in commits `69ffa299f` and `a50d35d4e`.
- Step 2 declaration/store fact capture is accepted in `8eca000c9`.
- The active packet is the Step 2 return from closed 852: migrate only the
  bounded `lir_owned_type_spec` function-signature occurrence producer to
  consume populated `QualType::aggregate_ref` through the accepted
  HIR-ref-to-LIR-ref intern relation.

## Non-Goals

- Do not close/resume 836 or 831, alter their return, or merge the incomplete,
  unmatched-owner, and no-owner-compatibility groups.
- Do not migrate function, vector, scalar, unions, generic verifier/printer,
  collectors, globals, or universal-model deletion work beyond the bounded
  function-signature occurrence producer named above.
- Do not use runtime text, tags, parser pointers, reconstructed owner keys,
  `record_def`, `Node*`, RTTI, universal-ID facades, or testcase-shaped
  exceptions.

## Execution Rules

- Keep each packet buildable and record packet evidence in `todo.md`.
- Register canonical definitions before occurrence lowering; unknown,
  incomplete, stale, foreign, and wrong-module refs fail closed.
- Preserve accepted 754/798/801/803 behavior. Delete legacy `record_def`,
  owner-key, tag/text lookup, and duplicate metadata only after every named
  declaration, field, call, verifier, printer, and receiver consumer migrates.
- For the current Step 2 return, consume existing HIR `QualType::aggregate_ref`
  facts directly and intern them through the accepted HIR-ref-to-LIR-ref seam;
  do not reconstruct missing identity from compatibility metadata.

## Ordered Steps

### Step 1 - Establish the canonical aggregate identity and store seam

Status: Accepted in `69ffa299f` and `a50d35d4e`.

Goal: identify the current HIR canonical owner/index and introduce the smallest
stable HIR-to-LIR aggregate ref/store contract.

Completion check: fresh build plus focused lowering proof demonstrated one
canonical definition-to-store mapping and explicit unknown/incomplete rejection.

### Step 2 - Preserve recursive aggregate facts and all aggregate forms

Status: Accepted in `8eca000c9`, Step 2 return commits through `88ccf591d`.

Goal: migrate kind, fields, layout, projections, and recursive children without
flattening semantic structure.

Actions:

- Preserve the accepted `build_type_decls` snapshots of ordered structured LIR
  field types, packed/opaque flags, and direct/byte-storage/union layout kind.
- Migrate only the bounded `lir_owned_type_spec` function-signature occurrence
  producer so supported aggregate return/parameter occurrences consume populated
  `QualType::aggregate_ref`.
- Intern that HIR ref into the module-owned LIR aggregate store through the
  existing HIR-ref-to-LIR-ref relation.
- Prove the current function-signature failure family moves from missing
  structured owner keys to canonical refs without adding tag/text/key fallback.
- Retain legitimate no-owner rendered compatibility only at its named consumer.

Completion check: focused build/test proof showed supported ordinary aggregate
function signatures lower through canonical refs, nested/repeated aggregate
parity remains intact, and unsupported/missing/foreign/corrupted cases fail
closed without competing operation-local authority.

### Step 3 - Enforce module ownership and migrate bounded consumers

Status: Active; baseline repair focused proof passed after rejected baseline
candidate following `97eb154af`; supervisor full-suite baseline review still
required.

Goal: make justified consumers use canonical store facts and fail closed.

Actions:

- Migrate bounded declaration, field, call, verifier, printer, and receiver
  consumers justified by the store seam.
- Prove foreign, wrong-module, stale, and malformed rejection; retain only
  named adapters with explicit deletion gates.
- Repair the rejected baseline expansion before further normal Step 3 or Step 4
  progress. The focused repair proof now passes for both observed families:
  variadic/`va_arg` backend failures where LLVM saw an unsized
  `%struct.__va_list_tag_` GEP base after the store-backed printer/receiver
  path dropped legitimate no-owner structured declarations when
  `aggregate_store` is nonempty, and a C++ inline-method member-context
  frontend failure where `lir_owned_type_spec` rejected a legitimate
  legacy/no-owner aggregate function type as missing a module owner.
- Preserve populated-ref fail-closed behavior while restoring legitimate
  no-owner/legacy-owner compatibility only at the named bounded adapters. Do
  not use tag/text/key lookup, rendered declarations, testcase names, or
  expectation downgrades as acceptance.

Completion check: fresh build plus focused valid/invalid proof preserves
754/798/801/803 seams and rejects cross-module/foreign refs. While the baseline
repair blocker is active, Step 3 is not accepted until the focused repro subset,
focused frontend/backend LIR coverage, backend checkpoint, and a fresh
full-suite baseline review prove the baseline failure set did not expand.

### Step 4 - Assess bounded convergence and hand off remaining work

Goal: verify A1's contract and document exact remaining deletion gates.

Actions:

- Confirm every introduced carrier has one owner and no text/key/tag recovery.
- Run selected focused proof and the proportional shared-surface checkpoint.
- Record remaining adapters without absorbing 836, later successors, 812/813,
  or 797.

Completion check: source acceptance has supervisor-accepted proof or this route
returns to plan-owner with exact unmet criteria.
