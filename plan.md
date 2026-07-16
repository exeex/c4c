# HIR Canonical Semantic Aggregate-Ref Binding Runbook

Status: Active
Source Idea: ideas/open/852_hir_canonical_semantic_aggregate_ref_binding.md
Switched from: ideas/open/813_lir_string_semantic_authority_completion_umbrella.md at its preserved Step 2 return point

## Purpose

Bind the existing sema canonical signature/type graph once to registered,
module-owned HIR aggregate definitions so ordinary HIR function return and
parameter `QualType` occurrences carry `HirAggregateRef` before LIR lowering.

## Goal

Deliver a production HIR semantic-type-to-definition/ref materialization seam
and an accepted return that lets 838 resume only its bounded Step 2
`lir_owned_type_spec` producer migration.

## Core Rule

Perform one HIR-owned binding after aggregate definition registration and
before function signature finalization. The issued `HirAggregateRef` is the
downstream authority. Do not reconstruct identity from `TypeSpec`, parser
pointers, `record_def`, owner keys, tags, rendered text, runtime strings, or
`Node*` maps, and do not introduce an unfed signature sidecar.

## Read First

- `ideas/open/852_hir_canonical_semantic_aggregate_ref_binding.md`
- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`
- `ideas/closed/837_lir_nominal_type_family_architecture.md`
- `ideas/closed/848_hir_aggregate_occurrence_canonical_ref_population.md`
- `ideas/closed/849_hir_function_signature_direct_aggregate_ref_carrier.md`
- `ideas/closed/851_hir_function_signature_definition_provenance_architecture_blocker.md`
- `src/frontend/sema/canonical_symbol.hpp`
- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/impl/lowerer.hpp`
- `src/frontend/hir/hir_build.cpp`
- `src/frontend/hir/hir_types.cpp`
- `src/frontend/hir/hir_functions.cpp`

## Working Model

```text
ResolvedTypeTable / CanonicalFunctionSig / CanonicalType
                         |
                         v
HIR-owned materialization binding after HirStructDef registration
                         |
                         v
Function.return_type.aggregate_ref / Param.type.aggregate_ref
                         |
                         v
838 later consumes the ref through the existing LIR intern seam
```

Keep nested aggregate semantics as typed child relations. Do not flatten them
into rendered field layouts or duplicate type trees.

## Current Scope

- Trace and use the ordinary free-function resolved canonical signature/type
  flow.
- Select and implement the smallest HIR-owned binding phase where canonical
  sema identity and registered `HirStructDef`/`HirAggregateRef` facts coexist.
- Populate return and explicit-parameter `QualType::aggregate_ref` in
  production.
- Cover named and representative supported nested, local, template, anonymous,
  and typedef/alias aggregates.
- Define fail-closed missing, incomplete, ambiguous, invalid, foreign,
  wrong-module, and registration-order behavior.
- Prove the HIR capability and publish the exact return to 838 Step 2.

## Non-Goals

- Do not change LIR stores, consumers, verifiers, printers, or implement 838's
  `lir_owned_type_spec` migration.
- Do not revive 848/849/851 as capability owners or add a parallel ordinary
  function-signature carrier.
- Do not broaden into scalar, vector, nominal function, union, array-family,
  Raw-BIR, or universal-model migration.
- Do not alter 836/831's independent residual-failure return chain.
- Do not weaken expectations, mark supported cases unsupported, or accept a
  test-only producer.

## Execution Rules

- Use AST-backed symbol inspection where appropriate; record exact production
  callsites and ordering before editing shared HIR construction.
- Keep implementation packets narrow and commit-ready. Update `todo.md` after
  each accepted packet; do not rewrite the source idea for routine progress.
- Require nearby same-feature coverage, not one named regression case.
- Treat missing semantic evidence as fail-closed. Record a separately scoped
  limitation rather than adding compatibility recovery.
- Use the validation ladder appropriate to each packet; shared HIR changes
  require a fresh build, focused HIR proof, and proportional HIR-to-LIR/backend
  proof before final acceptance.

## Ordered Steps

### Step 1 - Prove the semantic-to-HIR materialization seam

Goal: identify the exact production phase/API where canonical signature/type
facts and registered module-owned aggregate refs coexist.

Actions:

- Trace ordinary free-function return and parameter types from
  `ResolvedTypeTable` and `CanonicalFunctionSig`/`CanonicalType` into current
  `hir::Function` and `Param` construction.
- Trace all supported aggregate-definition registration routes and the point at
  which each `HirStructDef` receives its module-owned `HirAggregateRef`.
- Define the canonical semantic identity relation, module/lifetime ownership,
  ordering, recursive wrapper traversal, and the smallest HIR-owned binding
  API.
- Prove both return and parameter production sources exist. Reject a renamed
  text/key lookup, parser/`record_def` authority, `Node*` map, or sidecar with
  no production source.

Completion check: the selected seam can bind production canonical semantic
aggregate identity to an already registered definition/ref before signature
finalization, with explicit failure behavior and no downstream reconstruction.

### Step 2 - Materialize the binding and attach occurrence refs

Goal: populate definition-backed aggregate refs on ordinary HIR function
signature occurrences.

Actions:

- Implement the selected one-time HIR semantic-type-to-definition/ref binding.
- Validate module ownership, canonical equivalence, distinction of separate
  definitions, and registration order.
- Attach the resulting ref to ordinary free-function return and explicit
  parameter `QualType` occurrences while preserving pointer, array, nested,
  and other supported recursive type structure.
- Add focused production-path HIR coverage for return and parameter attachment,
  repeated canonical occurrences, and missing, ambiguous, incomplete,
  invalid, foreign, wrong-module, and use-before-registration boundaries.
- Run a fresh default build and the focused `frontend_hir_tests` proof for each
  accepted implementation packet.

Completion check: production construction populates valid module-owned refs
for supported return and parameter occurrences, failure cases remain explicit
and fail closed, and the exact unset-ref failure is not hidden behind a helper.

### Step 3 - Prove complex forms and hand back to 838

Goal: establish proportional cross-boundary proof and publish an executable
838 Step 2 return.

Actions:

- Add nearby coverage for named aggregates and representative supported nested,
  local, template, anonymous, and typedef/alias forms; preserve typed child
  relations rather than flattened rendered layouts.
- Run a fresh default build, focused `frontend_hir_tests`, and proportional
  HIR-to-LIR/backend tests that exercise ordinary aggregate function
  signatures and the relevant type-ref failure family.
- Audit production downstream paths to confirm none reconstruct aggregate refs
  from parser pointers, `record_def`, owner keys, tags, text, or `Node*` maps.
- Record the accepted commits/proof and exact return: reactivate 838 at its
  interrupted Step 2 and migrate only the bounded `lir_owned_type_spec`
  function-signature occurrence producer through the existing HIR-ref-to-LIR-ref
  intern relation.
- Confirm 839/843 remain downstream of 838 completion and 836 retains its own
  decomposition and `831 Step 4` return.

Completion check: all source acceptance criteria and reject-signal audits pass,
and 838 can resume without legacy recovery or LIR work being performed here.
