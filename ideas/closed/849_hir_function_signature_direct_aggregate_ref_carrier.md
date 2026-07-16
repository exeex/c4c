# HIR Function-Signature Direct Aggregate-Ref Carrier

Status: Intentionally concluded — no-change route
Type: bounded upstream HIR semantic-construction carrier prerequisite
Parent: `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`, Step 2b
Successor / return route: `ideas/open/851_hir_function_signature_definition_provenance_architecture_blocker.md`, then re-evaluate 848 Step 2b

## Intended Goal

Carry an already definition-backed, module-owned `HirAggregateRef` (or its
authoritative HIR definition) through production function-signature
construction to return and explicit-parameter lowering, without recovering
identity from normalized or parser-owned type metadata.

## Conclusion Record

This bounded route is intentionally concluded without implementation and does
not claim carrier delivery or aggregate occurrence population. The corrected
Step 2 production trace found that every `Lowerer::lower_function` call is in
`src/frontend/hir/hir_build.cpp:968,970,983,988,990,1064,1070` or
`src/frontend/hir/impl/stmt/decl.cpp:103,105`, and each supplies only
`Node*`, name, template, or NTTP overrides. No call has a `HirAggregateRef`,
`HirStructDef`, or another module-issued direct aggregate fact. Module-issued
refs are assigned only after HIR struct registration
(`src/frontend/hir/hir_types.cpp:3580-3581`; template instantiation around
`:564-566`), while method lowering reaches `lower_struct_method`, not
`lower_function` (`src/frontend/hir/hir_build.cpp:1005-1013`, with template
equivalents).

Consequently, adding the proposed carrier at this boundary would only
validate, hold, or discard an absent fact; making it deliver a fact requires a
forbidden `Node`/`TypeSpec`/record/tag/owner lookup or map. Test-only injection
would likewise not establish the production capability.

## Preserved Execution State

- Accepted Step 1: **Locate the signature semantic carrier seam** at
  `109ea13f4` (`plan: locate direct signature aggregate ref carrier seam`).
- Interrupted step: Step 2 — **Add the bounded direct carrier/API**. No Step 2
  code, tests, todo update, or post-change proof was produced or accepted.
- Related accepted parent work: 848 Step 2a is `359a9b94b`
  (`hir: add direct aggregate occurrence ref input`) with focused
  `frontend_hir_tests` proof.
- Prior no-change baseline: 850 intentionally concluded this adjacent
  producer-order premise at `1723df997` after a fresh default build and
  `frontend_hir_tests` passed. This conclusion adds no code proof and does not
  roll that baseline forward.

## Exact Return Point

851 must first decide whether a broader, legal HIR semantic-construction
architecture can create and retain definition provenance before function
signature normalization, or must record that supported aggregate function
signatures cannot obtain canonical occurrence refs under the current boundary.
It must not use parser/`TypeSpec`/record/tag/owner/text recovery, a `Node*`
map, test-only injection, `qtype_from` attachment, or LIR work. Only if 851
produces an explicitly approved legal direct-fact architecture and a separately
scoped implementation successor may 848 resume at unchanged Step 2b; otherwise
848's bounded function-signature route must be intentionally concluded rather
than retried.

## Reviewer Reject Signals

- Reject this conclusion being represented as a carrier implementation or
  aggregate occurrence-population capability.
- Reject parser/`TypeSpec`/record_def, owner, tag, text, parser-pointer,
  `Node*` maps, or reconstructed lookup as a direct semantic fact.
- Reject a carrier that only validates, stores, or discards an absent input;
  reject test-only delivery as production evidence.
- Reject `qtype_from` attachment, occurrence population, LIR work, or broad
  nominal-family changes claimed as work completed by this concluded route.
