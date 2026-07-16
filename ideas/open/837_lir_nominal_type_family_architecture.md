# LIR Nominal Type Family Architecture Umbrella

Status: Open
Type: Umbrella architecture classifier and follow-up idea generator
Parent: none
Handoff Directory: `docs/lir_nominal_type_family_architecture/`
Priority: First; execute this umbrella before the parked 836 route and before
refreshing 812/813
Related:
- `ideas/open/812_lir_string_authority_remaining_routes_umbrella.md`
- `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
- Historical design reference:
  `origin/new_bir:ideas/open/746_bir_node_kind_centric_storage_pass_contract.md`
  (removed from the current branch by `d424df250`; evidence only, not a
  lifecycle owner)

## Goal

Use current code and the historical LIR authority/blocker record to classify
every distinct semantic responsibility currently packed into universal
`LirTypeRef`, decide nominal family and restricted-polymorphism boundaries, and
generate an ordered set of separately scoped implementation ideas.

Preserve the approved architecture direction: retire the universal type bag in
favor of non-interchangeable scalar, vector, aggregate, and function-signature
families; use a bounded tagged union only at genuinely polymorphic first-class
value boundaries; construct structured families directly from source/HIR facts;
render one-way through family overloads; and eventually delete semantic
runtime-text/string escape hatches.

837 owns evidence classification, architecture boundary decisions, and
successor generation only. It does not implement type-family code or directly
own full migration completion.

## Why This Exists

The target is not a clean-slate redesign. Universal `LirTypeRef` accumulated
responsibilities because concrete routes repeatedly hit different walls:

- builtin identity and runtime spelling were initially coupled;
- call/signature mirrors needed structured precedence while raw compatibility
  stayed alive;
- array, named composite, anonymous aggregate, and vector facts appeared as
  separate structured islands;
- expression and coercion APIs lost value/type authority through strings;
- aggregate layout, value provenance, owner identity, and module ownership
  emerged as distinct contracts;
- operation-local vector/aggregate facts solved bounded rows without proving a
  coherent recursive type model;
- Raw-BIR receipt could proceed only after each LIR producer handoff existed.

Turning `LirTypeRef` into a larger variant, a universal table ID, or a struct
with more optional fields would preserve the same responsibility collision.
Deleting it in one rewrite would reopen accepted capability and obscure which
first owner broke. The umbrella must derive the extraction queue from those
historical walls and current callsites, then hand each coherent route to one
future source idea.

## Approved Architecture Direction

The classification must converge on these durable constraints:

- Scalar-only schemas use a compact nominal scalar family and cannot accept
  vector, aggregate, or function refs at compile time.
- Vector schemas use a nominal vector ref/store that owns lane and element
  shape; row-local vector metadata must not remain a competing authority.
- Aggregate schemas use a canonical module-owned aggregate ref/store that owns
  identity, layout, fields, projections, and owner coherence.
- Call/declaration schemas use a nominal function signature ref that composes
  only allowed value-type alternatives.
- Only evidence-backed polymorphic first-class-value boundaries such as call
  argument/result, PHI, select, or return may use a tagged union. Each boundary
  has an explicit enum kind and exact allowed alternatives, with custom
  C++20 `isa`/`cast`/`dyn_cast`-style checked access.
- No `std::dynamic_cast`, C++ RTTI, virtual type hierarchy, vtable-dependent
  dispatch, TableGen requirement, or external schema DSL is introduced.
- HIR-to-LIR constructs families directly from `TypeSpec`, canonical owner,
  layout, declaration, and signature facts, never by classifying `llvm_ty()`
  output or other rendered text.
- Verification, semantic dispatch, and LLVM rendering use overloads by nominal
  family. Rendering is one-way output/compatibility only.
- Temporary adapters are explicit, one-way, consumer-named, and have deletion
  conditions. No adapter may parse text back into semantic state.

The historical `origin/new_bir` Idea 746 `NodeKind Traits and Schema` section
is the design reference for enum tags, C++20 traits, `constexpr` schema, and
compile-time/runtime helper separation. It is not an open idea, implementation
dependency, or owner of LIR work.

## Required Evidence Inputs

Execution must refresh all evidence against one recorded current revision and
must inspect exact closure/resumption boundaries, not only idea titles.

### Foundation And String Boundaries

- 759: enum-first builtin foundation, explicit retention of runtime string
  construction, HIR `TypeSpec` rendering, parsed calls, extern text, and
  verifier/BIR fallback evidence.
- 760: completed constructor inventory/deprecation route and its retained named
  runtime-text boundaries, including parsed calls, extern declarations, inline
  asm types, aggregate/field/signature text, indexed GEP, aggregate padding,
  vector ABI, and va-list compatibility.
- 761: structured call/signature and switch type-mirror precedence, plus raw
  call compatibility and intentional inline-asm text exclusions.
- 763: builtin/integer, named struct/union, array, and selected structured
  composite islands; explicit deferred vector, aggregate literal, function,
  recursive, and `runtime_text` forms.

### Value, Aggregate, And Vector Walls

- 775/776: PHI producer helper result identity and decomposition evidence for
  expression-result/coercion string loss.
- 754: accepted representative ExtractValue, InsertValue, InsertElement,
  ExtractElement, and ShuffleVector rows; bounded operation capability, not a
  universal type-family closure.
- 798: selected aggregate operand provenance publication and its exclusions.
- 801: native anonymous aggregate layout/type facts and the exact structured
  call/layout handoff.
- 803: bounded aggregate SSA producer authority for selected local-load and
  constructed-insertvalue paths.
- 811: reusable native vector result/use, lane/element, index, and mask carrier;
  explicitly not operation-row semantics or generic vector type closure.
- 814/815: bounded poison second-shape and mask-lane coherence repairs; do not
  reopen them or generalize them beyond their accepted splat seams.

### Module, Call, Parameter, And Owner Walls

- 762: completed module declaration/type-shadow precedence and its exclusions
  for full type-tree conversion, HIR lowering rewrite, and global initializer
  semantics.
- 795: bounded body-parameter handoff; all ABI/byval/HFA/vector/variadic forms
  were not completed by that selected route.
- 829/830 and current return chain: next body-parameter authority depends on a
  native direct-call argument identity/type relation; call-signature facts and
  body-use identity are distinct owners.
- 832: bounded HIR aggregate-owner function-parameter crash repair.
- 833: bounded direct-scalar truthiness-LHS parameter authority.
- 834/835: durable HIR aggregate-owner identity and selected
  `lir_owned_type_spec` module-owner canonicalization relation.
- parked 836 evidence: residual structured-key, matching-module-owner, and
  no-owner compatibility failures were observed before the user-priority
  switch. 837 must preserve the exact 836 return obligation, use those failures
  as high-priority architecture evidence, and must not claim to satisfy 836.

### Receiver And Terminal Boundaries

- 734: typed Raw-BIR receipt only after an exact accepted LIR handoff; it does
  not own producer/type-model repair.
- 797: terminal coverage/dispatcher/proof convergence only after every valid
  LIR fact has a disposition; it is not a catch-all architecture owner.
- 812/813: current string-authority inventory/routing outputs where available;
  coordinate row keys and owner mappings without duplicating their umbrellas.

### Current Code

At minimum inspect:

- `src/codegen/lir/types.hpp`: `LirTypeRef` storage, kind classification,
  structured islands, `runtime_text`, mutable `str()`, conversions, equality,
  and rendering;
- `src/codegen/lir/ir.hpp`: every operation/module field typed as `LirTypeRef`
  and every parallel owner/layout/shape/signature mirror;
- HIR-to-LIR type construction, `llvm_ty()` use, call construction,
  `emit_rval_id`, `emit_rval_payload`, `coerce`, and raw operand/result seams;
- `src/codegen/lir/verify.cpp`, especially `require_module_type_ref` and every
  operation-family verification call;
- `src/codegen/lir/lir_printer.cpp`, call argument compatibility helpers,
  reference collectors, and LIR-to-BIR consumers.

## Required Umbrella Outputs

Later execution must create under
`docs/lir_nominal_type_family_architecture/`:

- `current_lir_type_ref_responsibility_matrix.md`: exact field/API/callsite,
  producer, consumer, current authority, text dependency, legal nominal family,
  existing owner, migration owner, and deletion condition;
- `historical_blocker_root_model_map.md`: each required historical wall mapped
  to the overloaded root responsibility it exposed and the accepted capability
  that must be preserved;
- `nominal_family_boundary_decisions.md`: scalar/vector/aggregate/function and
  bounded-union alternatives, stores, ownership, composition, and compile-time
  separation rules;
- `dependency_ordering.md`: schema/store prerequisites, producer migrations,
  verifier/dispatch/printer transitions, vertical proof order, active-owner
  dependencies, and deletion gates;
- `successor_queue.md`: generated idea paths, exact responsibility rows,
  first-owner scope, return points, proof, and order;
- `closure_trace.md`: evidence revision, generated successors, reused owners,
  deferred rows, and confirmation of no implementation inside 837.

## Responsibility Matrix Contract

Every responsibility currently packed into `LirTypeRef` or directly coupled to
it must receive one row or an explicitly linked row group. Each row records:

- exact fields, factories, helpers, and semantic callsites;
- current representation and whether text is storage, classifier, mirror, or
  final rendering;
- producer, verifier, dispatcher, printer, and receiver consumers;
- scalar, vector, aggregate, function-signature, bounded value-union,
  intentional render-text, or evidence-needed destination;
- module/local ownership, identity, layout/shape, and recursive child rules;
- accepted historical capability and tests that must not regress;
- existing open owner/dependency, if any;
- one future first owning layer and coherent successor contract;
- compatibility adapter and exact old API/field deletion conditions.

The matrix must recursively inspect child refs. An outer structured vector,
aggregate, array, call signature, or module carrier is not sufficient when its
element, field, return, parameter, or nested composite remains text-primary.

## In Scope

- Produce and reconcile the required evidence documents.
- Classify universal `LirTypeRef` responsibilities by first owner and nominal
  destination.
- Decide exact family/store/union boundaries from current evidence.
- Reuse current open owners as dependencies or evidence without expanding or
  duplicating them.
- Generate ordered successor ideas under `ideas/open/` only after the evidence,
  matrix, boundary, and dependency steps of this activated umbrella complete.
- Record staged, buildable vertical migration and final deletion order.

## Out Of Scope

- Implementing any type family, store, union, adapter, producer migration,
  verifier, dispatcher, printer, receiver, test, or deletion inside 837.
- Generating successors before the evidence, responsibility matrix, nominal
  boundary, and dependency-ordering steps are accepted.
- Implementing generated successors, broad testing, or committing inside 837.
- Revising or duplicating 812/813 or current 734/795/797/829-831/836 ownership.
- Reopening closed capabilities merely to rename types or churn representation.
- A universal ID façade, giant optional struct, unrestricted variant, RTTI
  hierarchy, external DSL, or big-bang implementation route.
- Parsing rendered text, names, operands, printer output, diagnostics, or
  testcase identity to construct or repair semantic refs.

## Priority Model

Order successors by first owning layer and dependency leverage:

1. Execute 837 first while preserving the parked 836 -> 831 Step 4 return
   obligation as durable state.
2. Generate the canonical module-owned aggregate ref/store successor as the
   highest-priority implementation route. Current 832-836 evidence makes
   aggregate owner/type identity the recurring first wall.
3. Order nominal function-signature/call composition next when it depends on
   aggregate/value-family facts, coordinating 761 and the parked 829/830 chain.
4. Order vector, scalar, restricted union, and direct HIR construction routes
   by demonstrated dependency and first-owner evidence rather than list order.
5. Move verification, dispatch, and printing to family overloads only after
   their native family producers/stores exist.
6. Keep universal `LirTypeRef`, semantic `runtime_text`, and string-conversion
   deletion terminal after every named consumer and compatibility adapter has
   an accepted replacement.
7. After the type-family successor queue and accepted capabilities are
   established, run 812 to refresh the broader string inventory, then 813 to
   route residual non-type string authority. Do not use stale pre-837 counts.

## Minimum Required Successor Families

Unless fresh evidence proves a different first-owner split, later execution
must generate and order at least these separately scoped families:

1. **Canonical module-owned aggregate ref/store convergence.** Own aggregate
   identity, layout, fields, projections, and owner relations; consume
   754/801/832-836 evidence and retire duplicate metadata only after parity.
2. **Nominal function signature ref and call/declaration migration.** Compose
   allowed value alternatives, coordinate closed 761 and current 829/830, and
   preserve raw compatibility only as an explicit one-way boundary.
3. **Vector nominal ref/store and vector operation migration.** Own vector
   lane/element shape and migrate bounded vector schemas while consuming closed
   754/811/814/815 evidence without reopening it.
4. **Compact scalar nominal type and scalar-only operation migration.** Define
   scalar identity and migrate bounded scalar-only schemas so vector,
   aggregate, and function refs are compile-time invalid.
5. **Restricted polymorphic first-class value union and traits.** Define exact
   alternatives per proven call/PHI/select/return boundary, explicit enum kind,
   exhaustive handling, and custom C++20 checked access without RTTI/vtables.
6. **Direct HIR-to-LIR structured family construction.** Replace semantic
   `llvm_ty()`/`runtime_text` construction with native `TypeSpec`/owner/layout/
   signature facts in bounded producer groups.
7. **Family-overloaded verifier, dispatch, and printer migration.** Replace
   universal `require_module_type_ref`, kind switching, and generic rendering
   with nominal overloads and one-way output.
8. **Universal model deletion and final convergence proof.** Remove old
   `LirTypeRef`, semantic `runtime_text`, mutable `str()`, implicit string
   conversions, semantic string comparison/classification, and expired
   adapters; require focused plus full shared-surface proof and feed the final
   disposition to 797.

Fresh evidence may split a family further. It may combine rows only when they
share one first owning layer, one coherent semantic contract, compatible proof,
and one deletion condition. Each generated idea must name the exact old
`LirTypeRef` responsibilities and APIs it owns, existing dependencies, accepted
capabilities it preserves, vertical proof, and parent return point.

## Successor Generation Rules

- One first owning layer and coherent semantic contract per idea.
- Existing open owners are dependencies/evidence, not duplicated successors.
- A successor may introduce only explicit one-way adapters with named consumers
  and deletion conditions.
- Each implementation route remains buildable after every bounded vertical
  slice; no all-files or all-operations big-bang packet.
- Each route includes compile/build proof, nearby positive and malformed/
  foreign/wrong-family coverage, and broader/full proof proportional to shared
  surface risk.
- Successor generation occurs only in the ordered runbook step after evidence,
  responsibility, boundary, and dependency outputs are accepted.

## Acceptance Criteria

- All required docs exist and agree on one current evidence revision.
- The responsibility matrix covers every current `LirTypeRef` responsibility,
  field, constructor/factory, semantic consumer, recursive child shape, and
  coupled owner/layout/signature mirror, with no unexplained runtime-text bucket.
- Every required historical closure/resumption record maps to a root-model
  responsibility, accepted capability to preserve, and successor dependency or
  explicit no-action disposition.
- The boundary decision defines compile-time nominal separation, vector shape
  ownership, canonical module-owned aggregate identity/layout/projection,
  function-signature composition, and exact bounded union alternatives.
- Generated successors cover all unresolved matrix rows exactly once, reuse
  existing owners, name old responsibility/deletion conditions, and follow the
  required first-owner/staged-proof split.
- Successor contracts require overload-based verification/dispatch/rendering,
  one-way output, malformed/foreign/wrong-family rejection, and explicit
  exhaustive union handling.
- The final queue has a deletion gate for universal `LirTypeRef`, semantic
  `runtime_text`, mutable/implicit string access, semantic text classification/
  comparison, duplicate operation-local shape/owner metadata, and expired
  adapters.
- 837 changes only handoff docs and generated source ideas during later
  execution; it performs no implementation, tests, runtime changes, active-plan
  changes, or direct capability migration.

Closure of 837 proves architecture classification and successor-generation
completeness only. It must not claim compile-time nominal separation or any
implementation capability has landed; those claims belong to generated ideas
and their accepted proof.

## Closure Note Requirements

The closure note must state the evidence revision, historical records and code
surfaces reviewed, every handoff document, responsibility row counts, boundary
decisions, existing owners reused, every generated successor and exact rows,
dependency order, deletion conditions, unresolved/deferred evidence, and final
797 relationship. It must explicitly state that 837 implemented no type-family
code and that classification/successor creation is not migration completion.

## Reviewer Reject Signals

- Reject direct implementation, tests, schema/store/union code, producer or
  verifier migration, printer changes, deletions, or active lifecycle changes
  inside 837.
- Reject an abstract clean-slate proposal that does not map 759/760/761/763,
  754/798/801/803/811/814/815, 762, 775/776, 795/829/830, 832-836, 734/797,
  and current callsites to exact root responsibilities.
- Reject a renamed universal bag, universal `LirTypeRef` ID façade, giant
  optional struct, or unrestricted variant that preserves runtime kind checks.
- Reject `std::dynamic_cast`, C++ RTTI, virtual hierarchy, vtable-dependent
  semantics, TableGen requirement, or external DSL.
- Reject unions without explicit bounded alternatives, enum kind, exhaustive
  handling, and custom checked access at a proven polymorphic boundary.
- Reject text classification/parsing, `llvm_ty()` output, names, operands,
  printer output, diagnostics, or testcase spelling as semantic authority.
- Reject duplicate operation-local vector shape, aggregate layout, owner-key,
  module-owner, anonymous-layout, or field authority instead of convergence on
  the nominal store contract.
- Reject reopening accepted capabilities for representation churn or creating
  successors that duplicate current open owners.
- Reject a broad big-bang successor without staged buildable vertical proof,
  exact old responsibilities, adapter consumers, and deletion conditions.
- Reject weakened verifier checks, reduced tests, expectation downgrades,
  unsupported markers, allowlists, filtering, or named-test fixes.
- Reject classification documents, helper renames, adapters, or successor
  creation claimed as compile-time separation or implemented migration.
- Reject closure while any responsibility row lacks one owner/disposition,
  dependency order, accepted-capability preservation rule, or deletion gate.
