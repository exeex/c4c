# HIR Aggregate Occurrence Canonical Ref Population Runbook

Status: Active
Source Idea: ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md
Supersedes: 838 Step 2 pending upstream HIR producer prerequisite

## Purpose

Close the upstream HIR producer gap that leaves aggregate occurrence
`QualType::aggregate_ref` unset, so 838 can later enforce canonical LIR
aggregate identity without fallback recovery.

## Goal

Attach definition-backed canonical `HirAggregateRef` values to supported
aggregate `QualType` occurrences, beginning with function returns and
parameters, and prove invalid/foreign/missing boundaries explicitly.

## Core Rule

Canonical occurrence identity must flow from registered HIR definitions. Do not
reconstruct it from owner keys, tags, rendered text, parser pointers, or
downstream LIR state.

## Read First

- `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`
- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- HIR aggregate-definition registration and `QualType` construction seams
- existing aggregate function-signature lowering coverage

## Scope

- HIR aggregate occurrence production, definition linkage, and boundary
  diagnostics/behavior needed to populate `QualType::aggregate_ref`.
- Function return and parameter occurrences first; only adjacent occurrence
  construction required to keep the producer contract coherent.

## Non-Goals

- Do not migrate `lir_owned_type_spec`, LIR aggregate store consumers,
  verifiers/printers, or 838 Step 2's LIR producer work.
- Do not add any legacy identity fallback or absorb 836/831 work.

## Execution Rules

- Keep a definition-backed module/id ref as the sole canonical authority.
- Treat missing, invalid, and foreign refs as explicit fail-closed cases.
- Keep packets narrow; build and run focused proof after each code-bearing
  packet, with a proportional backend checkpoint before handoff.

## Ordered Steps

### Step 1 - Locate the HIR definition-to-occurrence producer seam

Goal: identify where aggregate definitions receive stable IDs and where
aggregate `QualType` occurrences are created for function signatures.

Actions:

- Trace registration, module association, and all relevant return/parameter
  construction paths.
- Specify the direct definition-backed source for `HirAggregateRef` and the
  exact missing/invalid/foreign boundary behavior.
- Add only diagnostic or focused coverage scaffolding needed to demonstrate the
  current producer contract.

Completion check: the implementation seam and fail-closed contract are explicit
without changing LIR lowering or using legacy recovery.

Accepted discovery: `Lowerer::qtype_from` in
`src/frontend/hir/hir_types.cpp:469` constructs `QualType`, including legacy
owner identity; function returns and parameters call it at
`src/frontend/hir/hir_functions.cpp:543` and `:1157`. HIR exposes
`Module::register_aggregate_definition` / `aggregate_ref_for_definition` in
`src/frontend/hir/hir_ir.hpp:2611` / `:2618`, while the current registration is
only in downstream `lir::lower` at
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1804-1805`. Therefore HIR-side
definition registration must precede `qtype_from` resolving the existing HIR
definition-backed ref. If no direct registered ref exists, or it is not
module-owned and complete, leave `aggregate_ref` unset; do not recover it from
legacy owner/tag/parser/text state.

### Step 2a - Establish a direct HIR aggregate-ref construction input

Goal: create the legal HIR construction path before any signature occurrence
receives a canonical ref.

Actions:

- Register each module-owned `HirStructDef` at its HIR construction seam and
  retain the issued `HirAggregateRef` in a HIR-only definition/carrier path.
- Extend `qtype_from` with an explicit optional canonical-ref input; it may
  validate and copy only that direct input, never derive identity from
  `TypeSpec` or `record_def`.
- Require the input to be complete and `module_`-owned; otherwise leave
  `QualType::aggregate_ref` unset.
- Do not add a `Node* -> HirStructDef`/ref map and do not obtain a ref through
  owner-key, tag, rendered-text, or parser-pointer lookup.

Completion check: a direct, definition-issued, module-owned ref can reach
`qtype_from` without parser-owned identity recovery; a fresh build and focused
HIR proof pass.

### Step 2b - Populate canonical refs for aggregate function signatures

Goal: make supported aggregate function return and parameter occurrences carry
the registered definition's canonical ref.

Actions:

- Pass only the Step 2a direct HIR construction input for supported function
  return and parameter occurrences; do not add a lookup inside `qtype_from`.
- Preserve legitimate supported aggregate forms within the producer's scope.
- Add nearby coverage that observes propagation through both return and
  parameter occurrences.

Completion check: focused tests show definition-backed canonical refs on both
signature occurrence positions; a fresh build passes.

### Step 3 - Prove boundary behavior and hand off to 838

Goal: demonstrate that invalid producer inputs fail closed and record the exact
downstream return.

Actions:

- Cover missing, invalid, and foreign/module-mismatched refs at the producer
  boundary without fallback reconstruction.
- Run the focused HIR/lowering proof and proportional backend checkpoint.
- Record accepted evidence and state that 838 resumes only at its Step 2
  `lir_owned_type_spec` function-signature occurrence-producer migration.

Completion check: supervisor-accepted proof supports the 838 handoff and no
LIR migration was absorbed.
