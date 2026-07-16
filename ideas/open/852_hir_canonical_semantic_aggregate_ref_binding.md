# HIR Canonical Semantic Aggregate-Ref Materialization Binding

Status: Open
Type: upstream HIR semantic-type materialization prerequisite
Directly Blocked Parent:
- `ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md`, Step 2
Architecture Evidence:
- `ideas/closed/837_lir_nominal_type_family_architecture.md`
- `ideas/closed/848_hir_aggregate_occurrence_canonical_ref_population.md`
- `ideas/closed/849_hir_function_signature_direct_aggregate_ref_carrier.md`
- `ideas/closed/851_hir_function_signature_definition_provenance_architecture_blocker.md`

## Goal

Add the missing HIR-owned canonical binding between the existing sema type
backbone (`sema::CanonicalType`, `CanonicalFunctionSig`, and
`ResolvedTypeTable`) and the existing module-owned HIR aggregate backbone
(`hir::Module`, `HirStructDef`, and `HirAggregateRef`). Perform that binding
after aggregate definitions have registered their refs and before ordinary
`hir::Function::return_type` and `Param::type` occurrences are finalized, so
their `QualType::aggregate_ref` fields carry definition-backed, module-owned
identity into HIR-to-LIR lowering.

Completion must give 838 an exact executable return to its interrupted Step 2:
resume only the bounded `lir_owned_type_spec` function-signature occurrence
producer migration using populated `QualType::aggregate_ref` and the already
accepted HIR-ref-to-LIR-ref intern seam, without legacy identity recovery.

## Why This Exists

The compiler already has the containers needed to preserve this information:

- sema owns canonical type trees and function signatures, and
  `ResolvedTypeTable` relates semantic occurrences to those trees;
- HIR owns aggregate definitions and issues module-scoped `HirAggregateRef`
  values when those definitions are registered;
- ordinary HIR functions already carry return and parameter `QualType`
  occurrences that are consumed by HIR-to-LIR lowering.

The missing capability is the one-time HIR materialization relation between
the semantic canonical aggregate identity and the registered HIR definition/ref.
Today ordinary function signature construction converts back through
`TypeSpec`, leaving `QualType::aggregate_ref` unset; downstream tag, owner-key,
parser-pointer, and rendered-string compatibility then obscures that missing
binding.

848 and 849 correctly rejected adding another carrier whose production caller
had no direct ref to carry. 851 correctly concluded that its narrower contract
could not require every `lower_function` caller to arrive with an already
issued `HirAggregateRef`. This idea changes that over-narrow boundary: HIR may
perform one explicit materialization-time binding using sema canonical identity
and already registered HIR definition facts. The resulting `HirAggregateRef`,
not the lookup inputs, becomes the sole downstream occurrence authority.

## Architectural Contract

The target flow is:

```text
ResolvedTypeTable / CanonicalFunctionSig / CanonicalType
                         |
                         v
HIR-owned one-time semantic-type-to-definition binding
  (after HirStructDef registration, before signature finalization)
                         |
                         v
hir::Function::return_type.aggregate_ref
hir::Param::type.aggregate_ref
                         |
                         v
HIR-to-LIR consumes HirAggregateRef and interns LirAggregateRef once
```

The binding phase may consume canonical sema identity and registered HIR
definition facts to establish the relation. It must not turn a spelling,
rendered tag, owner key, parser pointer, `Node*` map, or `record_def` pointer
into a lasting canonical authority. After materialization, downstream
`QualType`, LIR producers, verifiers, printers, and receivers must consume the
issued ref and must not reconstruct identity.

Do not flatten nested aggregate semantics. Canonical definitions remain
module-owned nodes, and nested/pointer/array/function-signature type structure
retains typed child relations whose aggregate leaves bind to their registered
definitions.

## In Scope

- Inventory the existing production flow from `ResolvedTypeTable` and
  `CanonicalFunctionSig`/`CanonicalType` through ordinary free-function return
  and explicit-parameter construction into `hir::Function` and `Param`
  `QualType` values.
- Define one HIR-owned materialization phase/API that runs after relevant
  `HirStructDef` values have been stored and issued `HirAggregateRef` values,
  and before function signature occurrences are finalized or emitted.
- Establish the canonical semantic-type-to-registered-definition/ref relation
  once in that phase, with an explicit lifetime and module-ownership contract.
- Populate `QualType::aggregate_ref` for ordinary free-function aggregate
  return and parameter occurrences from the resolved canonical signature/type
  flow rather than requiring each `lower_function` caller to provide a ref.
- Preserve recursive semantic structure and cover named aggregates plus
  representative nested, local, template, anonymous, and typedef/alias forms
  wherever current sema/HIR evidence declares those forms supported. Record an
  exact separately scoped limitation instead of inventing fallback authority
  for a form that lacks canonical definition evidence.
- Specify and test missing canonical type, incomplete definition, invalid ref,
  foreign ref, wrong-module ref, ambiguous binding, and registration-order
  behavior. Unsupported or incoherent relations fail closed and remain
  distinguishable.
- Add focused nearby HIR and HIR-to-LIR coverage for production free-function
  return/parameter propagation and representative complex aggregate forms.
- Publish the exact accepted handoff that lets 838 resume its bounded Step 2
  producer migration without owner-key, tag, parser, or rendered-text recovery.

## Out Of Scope

- Changing the LIR aggregate store, LIR consumers, verifier/printer migration,
  or implementing 838's interrupted `lir_owned_type_spec` migration here.
- Adding a parallel ordinary-function signature sidecar, or reviving 848/849's
  carrier route when no production semantic source feeds it.
- Treating a tag, rendered string, reconstructed owner key, parser pointer,
  `record_def`, `Node*` map, or runtime-text lookup as lasting canonical
  identity.
- Parsing or classifying LIR/LLVM output to recover semantic aggregate facts.
- Broad scalar, vector, function-family, value-union, array-family, verifier,
  printer, receiver, or universal-model migration.
- Reopening 848, 849, or 851 as if their intentionally concluded routes had
  implemented function-signature occurrence population.
- Completing or changing 836/831's independent residual-failure return chain,
  weakening supported behavior, or changing tests to accept missing refs.
- Activating this idea or changing the current 813 `plan.md`/`todo.md`
  lifecycle as part of idea creation.

## Execution Shape

### Step 1 - Prove the existing semantic-to-HIR materialization seam

- Trace ordinary free-function signature types from `ResolvedTypeTable` and
  canonical function/type nodes through current HIR construction.
- Trace every supported aggregate-definition registration route and the point
  at which its module-owned `HirAggregateRef` becomes available.
- Select the smallest HIR-owned phase/API where both facts coexist. Record the
  exact semantic identity relation, ownership rules, ordering, and recursive
  type traversal before implementation.

Completion check: a production source exists for both return and parameter
occurrences, and the proposed relation is canonical semantic identity to an
already registered definition/ref, not a renamed text/key lookup or an empty
sidecar.

### Step 2 - Implement the one-time HIR binding and occurrence attachment

- Materialize and validate the semantic-type-to-definition/ref relation in the
  selected HIR phase.
- Attach the resulting module-owned ref to ordinary function return and
  parameter `QualType` occurrences, preserving wrappers and recursive child
  structure.
- Keep missing, ambiguous, incomplete, invalid, foreign, wrong-module, and
  registration-order failures explicit and fail closed.

Completion check: production signature construction, not test injection,
populates definition-backed `aggregate_ref` values for both ordinary return
and parameter paths; downstream reconstruction is unnecessary.

### Step 3 - Prove supported complex forms and publish the 838 return

- Add nearby same-feature coverage for named aggregate return/parameter pairs
  and representative supported nested/local/template/anonymous/typedef-alias
  forms, including recursive child relations where applicable.
- Prove invalid, missing, foreign, wrong-module, ambiguous, and ordering
  boundaries without expectation downgrades.
- Run a fresh default build, focused `frontend_hir_tests`, and proportional
  HIR-to-LIR/backend coverage that exercises ordinary aggregate function
  signatures, including the relevant type-ref failure family.
- Record the exact accepted proof and handoff to 838 Step 2.

Completion check: 838 can resume at its existing interrupted Step 2 and migrate
only the bounded `lir_owned_type_spec` function-signature producer to consume
`QualType::aggregate_ref` through the existing LIR intern relation. No legacy
recovery is needed or authorized.

## Acceptance Criteria

- Ordinary free-function aggregate returns and explicit parameters receive
  valid, definition-backed `HirAggregateRef` values through the production
  canonical sema-to-HIR materialization flow.
- The returned refs are owned by the same HIR module as their registered
  `HirStructDef`; repeated equivalent occurrences resolve to the same canonical
  definition/ref, while distinct definitions remain distinct.
- Named aggregates and representative supported nested, local, template,
  anonymous, and typedef/alias forms have nearby coverage; nested structures
  remain typed reference graphs rather than flattened rendered layouts.
- Missing canonical data, incomplete definitions, ambiguous relations,
  invalid/foreign/wrong-module refs, and use-before-registration have explicit
  fail-closed behavior and focused tests.
- No production downstream path needs parser pointers, `record_def`, owner
  keys, tags, rendered strings, runtime text, or `Node*` maps to reconstruct an
  aggregate occurrence ref.
- A fresh default build plus focused HIR and proportional HIR-to-LIR/backend
  tests pass without weakening existing contracts or relying on one named
  testcase alone.
- The completion record returns directly to 838 Step 2 at its bounded
  `lir_owned_type_spec` function-signature producer migration, preserving the
  accepted declaration/store facts and existing HIR-ref-to-LIR-ref intern seam.
- No other parent is falsely claimed as directly unblocked: 839 and 843 remain
  downstream of 838 completion, while 836 retains its independent decomposition
  and `831 Step 4` return obligations.

## Reviewer Reject Signals

- Reject a renamed tag/string/owner-key lookup, parser-pointer recovery,
  `record_def` authority, `Node*` map, or rendered-text classifier presented as
  canonical semantic binding.
- Reject a sidecar or carrier whose production source is absent, or a test-only
  injection presented as proof that ordinary function construction populates
  aggregate refs.
- Reject scope creep into LIR store/consumer/verifier/printer implementation or
  claiming 838's producer migration as work completed by this HIR prerequisite.
- Reject retaining the exact current unset-`QualType::aggregate_ref` failure
  behind a new helper, registry, or API name.
- Reject testcase-shaped matching, one-case-only fixes, expectation downgrades,
  supported-to-unsupported changes, or weaker missing/foreign/module behavior.
- Reject flattening nested aggregates into rendered field text or duplicate
  layouts instead of preserving canonical typed child relations.
- Reject a broad nominal-family rewrite, unrelated cleanup, or changes to the
  active 813 lifecycle under this source.
- Reject claiming that closed 848/849/851 implemented this capability; they are
  evidence explaining why the materialization boundary must be different.
