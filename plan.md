# LIR Global Policy And Symbol-Identity Evidence Runbook

Status: Exhausted - Pending Plan-Owner Decision
Source Idea: ideas/open/848_lir_global_policy_identity_evidence.md
Activated after: ideas/open/734_lir_to_new_bir_container_completeness.md post-Step 7.52 close rejection

## Purpose

Produce the documentation evidence required before 734 or 797 can consume or
converge global linkage, visibility, qualifier-policy, and symbol-identity
facts.

## Goal

Trace every in-scope `LirGlobal` policy and identity field from producer
through verifier boundary to any new-BIR receiver disposition without using
rendered declaration text as authority.

## Core Rule

This is research and architecture documentation only. Do not edit
implementation files, tests, expectations, unsupported markers, allowlists,
runtime behavior, or lifecycle history beyond this active plan.

## Read First

- `ideas/open/848_lir_global_policy_identity_evidence.md`
- `docs/lir_string_authority_remaining_routes/handoff_to_813.md`
- `docs/lir_string_semantic_authority_completion/input_validation.md`
- `ideas/closed/844_lir_global_extern_initializer_family_facts.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/open/797_lir_to_new_bir_final_coverage_convergence.md`

## Current Scope

- Native global linkage, visibility, qualifier-policy, and symbol-identity
  facts.
- Producer, verifier, and new-BIR receiver traces for each field.
- Positive evidence and applicable malformed, foreign, or mismatch evidence
  expectations.
- A return relation naming what 844 can reuse, what 734 may receive only after
  an exact handoff, and what 797 must await.

## Non-Goals

- No C++ implementation changes.
- No test contract changes or proof expectation downgrades.
- No `llvm_type`/declaration-shadow rows, absent-metadata compatibility,
  initializer scans, or global/extern type facts already owned by 844.
- No inference from declaration rendering, display spelling, or link-name
  presentation.
- No direct 734 receiver or 797 convergence claim unless the evidence names an
  exact accepted handoff.

## Working Model

848 is the next ordered first-owner successor after 734 consumed the 867
`va_start` handoff. It may produce evidence and, if justified, a precise return
condition for later receiver work. It does not implement that receiver work.

## Execution Rules

- Create exactly the required documentation directory and files:
  `docs/lir_global_policy_identity_evidence/index.md` and
  `docs/lir_global_policy_identity_evidence/01_global_policy_identity_route.md`.
- The numbered answer file must contain the field-by-field evidence; the index
  only summarizes and links it.
- Use source locations and concrete traces instead of broad statements.
- Report missing evidence explicitly.
- Keep lifecycle proof to documentation/file checks unless the source is
  revised by a later supervisor decision.

## Ordered Steps

### Step 1 - Inventory Global Policy And Identity Fields

Status: Complete

Goal: identify the exact in-scope `LirGlobal` fields and their producer-facing
definitions.

Actions:
- Inspect the LIR global data structures, producer sites, verifier checks, and
  new-BIR import surfaces.
- Record every linkage, visibility, qualifier-policy, and symbol-identity field
  relevant to the routing key.
- Exclude 844-owned type and initializer facts explicitly.

Completion check:
- The evidence file contains a complete in-scope field inventory with source
  locations and explicit exclusions.

### Step 2 - Trace Producer, Verifier, And Receiver Routes

Status: Complete

Goal: answer the required diagnostic question for each field.

Actions:
- For each field, trace producer assignment, verifier or consistency boundary,
  importer/new-BIR receiver disposition, and any known test evidence.
- Separate proven native coverage from missing evidence.
- State malformed, foreign, or mismatch evidence expectations where the field
  has identity or consistency semantics.

Completion check:
- `01_global_policy_identity_route.md` contains a field-by-field route table or
  equivalent structured sections covering producer, verifier, receiver, and
  evidence status.

### Step 3 - Write Return And Dependency Conclusions

Status: Complete

Goal: define how the evidence affects 844, 734, and 797 without changing their
scope.

Actions:
- State what 844 can reuse as global-family context.
- State whether 734 has no handoff, one exact receiver handoff, or a missing
  prerequisite.
- State what 797 must await before final convergence.
- Make declaration-rendering non-authority explicit.

Completion check:
- The numbered answer and index agree on the dependency/return relation and do
  not claim implementation progress.

### Step 4 - Validate Documentation Shape

Status: Complete

Goal: prove the research deliverable is complete and bounded.

Actions:
- Check that `docs/lir_global_policy_identity_evidence/` contains exactly
  `index.md` and `01_global_policy_identity_route.md`.
- Run `git diff --check`.
- Run any lightweight text/link checks the repository already provides if a
  matching docs command exists.

Completion check:
- Required files exist with the required shape, no extra files are present in
  the directory, `git diff --check` passes, and no implementation files are
  modified.

Result:
- Created `docs/lir_global_policy_identity_evidence/index.md`.
- Created
  `docs/lir_global_policy_identity_evidence/01_global_policy_identity_route.md`.
- Recorded that Raw-BIR receiver coverage exists for global policy facts, but
  LIR verifier evidence is missing for the policy/string-carried fields, so no
  direct 734 receiver handoff is authorized by 848.
- Proof passed with exact required-file listing and `git diff --check`.
