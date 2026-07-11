# Debug, Proof, And Diagnostic Boundaries

This document answers Step 3's proof-boundary question: which prepared facts
are lowering inputs, which facts are verifier/proof material, which facts are
debug-only observations, and how `PreparedMirView` should prevent diagnostic
artifacts from becoming semantic authority.

The evidence baseline is the dependency inventory in
`01_current_mir_dependencies_on_prepared_bir.md`, the core-view proposal in
`02_prepared_mir_core_view_shape.md`, and the optional-feature split in
`03_feature_views_and_optional_contracts.md`. The current raw source shape is
`prepare::PreparedBirModule` in `src/backend/prealloc/module.hpp:32`, where
semantic BIR, target identity, prepared facts, optional plans, invariants,
completed phases, notes, and private producer route state currently live in one
aggregate.

## Fact Categories

`PreparedMirView` should use explicit categories instead of exposing the whole
prepared module and relying on naming convention.

| Category | Meaning | May MIR codegen consume it? | Examples |
| --- | --- | --- | --- |
| `RequiredFact` | A fact without which the target cannot correctly lower an admitted function or module. It has stable ids, cursor ownership, and producer-independent semantics. | Yes, through `PreparedMirCoreView` or a required optional feature view. | Target identity, function ids, prepared control flow, value homes, stack layout, addressing records, call plans for call cursors, edge-publication move records, carrier completeness for admitted special features. |
| `OptionalFeatureFact` | A `RequiredFact` only when a feature is present or an output mode requires it. Absence is allowed only when the feature is absent or the target rejects the unit. | Yes, after an explicit presence/admission check. | Calls, variadic entry, i128/f128 carriers, atomics, intrinsics, inline asm, object data, runtime-helper boundaries. |
| `VerifierFact` | A structured proof or coherence result that states whether required or optional facts are present, complete, and mutually consistent. It can fail closed or explain ownership of a missing fact. | Only as an admission gate or failure reason; it must not be used to synthesize instruction operands. | `PreparedContractVerificationReport` with `fact_family`, `owner_class`, `fail_closed`, and `detail` in `src/backend/prealloc/prepared_contract_verifier.hpp:617`; route-contract classifiers such as `verify_prepared_frame_slot_value_source_route_contract()` and `verify_prepared_local_frame_address_materialization_source_route_contract()` in `src/backend/prealloc/prepared_contract_verifier.cpp:1113`. |
| `ProofCertificateFact` | A producer-side proof artifact used to derive or validate a required fact, especially where the old pipeline records route-local proof state. | Not directly. It may be exposed only through a typed verifier/proof view, and accepted lowering must depend on the derived required fact. | Local-array selected proof edge paths, interval effects, range proofs, proof facts, checker inputs, local-address provenances, and scalar-local-load records populated in `BirPreAlloc::publish_contract_plans()` at `src/backend/prealloc/prealloc.cpp:43`. |
| `DiagnosticFact` | Rendered text, debug counters, route summaries, route names, dump focus filters, notes, or completed-phase markers. It helps humans inspect a route. | No. It may observe ids or facts but cannot select lowering behavior. | `completed_phases`, `notes`, and private `route_` in `PreparedBirModule`; prepared printer route/phase/note text in `src/backend/prealloc/prepared_printer.cpp:42`; backend route-debug focus filtering in `src/backend/backend.cpp:306`; x86 route summary/trace text in `src/backend/mir/x86/debug/debug.cpp:425`. |
| `CompatibilityFact` | A temporary old-route or target-local fact retained to keep existing code working while the view boundary is introduced. | Only if promoted into a named required or optional view with a migration owner. | x86 Route 6 call-use source index in `ConsumedPlans`, raw `PreparedBirModule*` in AArch64 `FunctionLoweringContext`, compatibility projection vectors, and route-local fast-path lane strings. |

The most important rule is that `VerifierFact`, `ProofCertificateFact`, and
`DiagnosticFact` can reject, explain, or compare prepared facts, but they
cannot become operands, labels, symbols, storage homes, call ABI assignments,
or feature admission by themselves.

## Route-Family Classification

| Route family | Current evidence | Category boundary | Required view outcome |
| --- | --- | --- | --- |
| Publication and edge moves | Step 1 identifies value locations, block-entry publications, and prepared lookup indexes as required core inputs. x86 already consumes edge-publication lookup records in `consume_edge_publication_move_intent()` at `src/backend/mir/x86/prepared/dispatch.cpp:55`. | The move/publication record is a `RequiredFact` when the target emits the edge move. Lookup status and missing-publication states are verifier/admission facts. Rendered instruction text in `EdgePublicationMoveIntent` is target-local output, not reusable prepared authority. | `PreparedMirFunctionView` should expose typed edge-publication records and value homes through core lookups. A target may fail closed on missing or unsupported publication status, but must not recover moves from diagnostic strings or note text. |
| Freshness and move-bundle source authority | `PreparedObjectMoveBundleConsumerStatus` names missing, invalid, ambiguous, and unsupported source freshness states in `src/backend/prealloc/prepared_object_traversal.hpp:43`; AArch64 emits object-consumer diagnostics from these statuses in `src/backend/mir/aarch64/codegen/traversal.cpp:22`. | Freshness identity attached to a selected move or value home is a `RequiredFact`. Ambiguity/missing statuses and messages are `VerifierFact`/`DiagnosticFact`. | The view should expose a typed move-bundle source-freshness record only when it is complete. Diagnostics may report `missing_move_bundle_source_freshness`, but codegen must not treat that category string as permission to guess a source. |
| Provenance and memory authority | Prepared addressing records include memory provenance, base identity, local/global/string authority helpers, and pointer-base-plus-offset checks under `src/backend/prealloc/addressing.hpp`; verifier families include `MemoryAccessProvenance` in `PreparedContractFactFamily` at `src/backend/prealloc/prepared_contract_verifier.hpp:36`. | Provenance that identifies the base, offset, width, alignment, and local/global authority for an admitted memory operation is a `RequiredFact`. A verifier report that says provenance is missing or conflicting is a gate. | `PreparedMirCoreView::addressing()` should provide provenance-bearing records. A target may not substitute note text, route names, or BIR operand spelling when a required provenance record is absent. |
| Local-array proof route | `publish_contract_plans()` populates local-array selected proof edge paths, endpoint bridges, ordered effect streams, interval effects, range proofs, proof facts, checker inputs, local-address provenances, semantic GEPs, and scalar local loads in `src/backend/prealloc/prealloc.cpp:43`. | The final local address provenance, semantic GEP, or scalar-local-load fact needed by lowering is a `RequiredFact` or optional local-array feature fact. The selected proof paths and range/checker inputs are `ProofCertificateFact`. | A future local-array view should expose the derived lowering fact and a separate proof/verifier handle. MIR should not branch on which proof path was selected except through a typed verifier outcome. |
| Select-chain direct-global/source routes | Store-source publication planning records direct-global select-chain source fields in `src/backend/prealloc/publication_plans.cpp` and AArch64 call lowering has select-chain materialization sites in `src/backend/mir/aarch64/codegen/calls.cpp`. | The selected source producer and materialization cursor are required only for a feature that emits that source. The textual route reason and root-is-select detail are proof/provenance unless promoted into a typed source dependency. | The view should expose a `PreparedMirSourceDependencyView` or feature-local source record when needed. It should not expose select-chain route names as lowering switches. |
| Route summaries and target-local debug | Backend dump routing sends `MirSummary` and `MirTrace` to x86 route debug in `src/backend/backend.cpp:354`; x86 debug renders grouped authority and Route 6 call-source statuses in `src/backend/mir/x86/debug/debug.cpp:59` and `:425`. | All rendered route summary and trace text is `DiagnosticFact`. The prepared ids and call plans it observes remain required or optional facts elsewhere. | Debug views should be named `PreparedMirDiagnosticView` or `PreparedMirRouteTraceView` and should be unavailable to ordinary lowering code. |
| Completed phases, notes, and producer route | `completed_phases`, `notes`, and private `route_` are fields of `PreparedBirModule`; the prepared printer renders route, completed phases, invariants, and notes. Backend filters explicitly describe focus strings and notes as debug text in `src/backend/backend.cpp:306` and `:805`. | `DiagnosticFact`. Invariants can be preconditions or verifier input, but phase strings and notes are not semantic facts. | `PreparedMirView` may expose a diagnostic-only phase/note stream for dumps. Core and feature views must not expose these fields. |

## Current Target-Consumer Risks

The current code already contains useful comments that fence off diagnostic
paths, but the type system does not enforce the boundary because targets still
receive `PreparedBirModule` or target contexts derived from it.

- `src/backend/backend.cpp:306` says route-debug focus options may resolve
  rendered spellings back to prepared ids only to trim debug output. That
  should remain a dump-only path; lowering admission should receive ids from
  the core view, not from focus strings.
- `src/backend/backend.cpp:805` says prepare notes are rendered diagnostics.
  Because `filter_prepared_module_to_function()` mutates `notes` while also
  filtering structured state, a future view should keep note filtering inside
  diagnostic rendering rather than in any lowering adapter.
- `src/backend/mir/x86/debug/debug.cpp:59` renders Route 6 scalar call
  argument source statuses and gates. This is useful proof output, but the
  corresponding lowering helper in `src/backend/mir/x86/x86.hpp:42` is named
  `ConsumedScalarI32CallArgumentSourceAuthority` and combines BIR Route 6
  records with prepared call plans. If generalized, this should become a typed
  verifier/check view or a promoted `RequiredFact`, not a debug route.
- `src/backend/mir/x86/debug/debug.cpp:142` renders grouped call/frame/storage
  authority by reading frame, call, regalloc, and storage plans. Those
  summaries must not become the way target code recovers register spans.
- `src/backend/mir/x86/prepared/dispatch.cpp:31` classifies a fast path using
  a public `focus_function` selector and returns lane/reason text. The comment
  states that the selector is not symbol authority. The view boundary should
  make this impossible by keeping focus strings in diagnostics.
- `src/backend/mir/aarch64/module/module.hpp:65` allows
  `ModuleLoweringDiagnostic` to hold a prepared object-consumer category and a
  `PreparedContractVerificationReport`. That is the right diagnostic payload,
  but codegen must not inspect `message` or `detail` text to choose an
  instruction sequence.
- `src/backend/mir/aarch64/module/module.hpp:80` still stores a raw
  `PreparedBirModule*` in `FunctionLoweringContext`. Until migrated, that raw
  pointer can bypass any diagnostic/required-fact split.

## Proposed Type And Naming Boundaries

The view contract should make each boundary visible in the type name and
namespace:

```cpp
namespace c4c::backend::mir::prepared {

class PreparedMirCoreView;
class PreparedMirFunctionView;
class PreparedMirFeatureSet;

template <class T>
class RequiredFactRef;

template <class T>
class OptionalFeatureFactRef;

struct PreparedMirVerificationReport;
class PreparedMirVerifierView;

class PreparedMirDiagnosticView;
class PreparedMirRouteTraceView;
class PreparedMirProofCertificateView;

}  // namespace c4c::backend::mir::prepared
```

Recommended conventions:

- lowering APIs should accept `PreparedMirFunctionView` and named feature
  views, never `PreparedMirDiagnosticView`;
- verifier APIs may return `PreparedMirVerificationReport` with stable enums,
  owner class, `fail_closed`, and ids, but any human `detail` text should be
  inside a `diagnostic_text` member that is not available in codegen-only
  headers;
- fields named `route`, `trace`, `summary`, `note`, `phase`, `debug`,
  `focus`, `reason`, or `detail` should live only under diagnostic/proof
  namespaces unless a design review explicitly promotes them;
- proof certificate records should name both their derived required fact and
  their proof owner, for example
  `PreparedMirProofCertificateView::local_array_range_proof(cursor)`, so a
  consumer cannot confuse proof provenance with the required address record;
- compatibility records should be prefixed with `Compat` or `Legacy` and have
  a deletion/migration owner before they can be exposed by a view.

## Reviewer Reject Rules

Reviewer tooling and manual reviews should reject a `PreparedMirView` migration
or target patch when any of these are true:

- MIR lowering APIs accept `PreparedBirModule`, `PreparedMirDiagnosticView`,
  `PreparedMirRouteTraceView`, rendered dump text, focus strings, route names,
  completed phases, or prepare notes as semantic input.
- Emitted operands, labels, register names, stack offsets, call ABI resources,
  publication moves, or memory addresses are derived from diagnostic strings,
  verifier `detail` text, route summary lines, or phase/note content.
- A verifier report's `detail` text or diagnostic category string is parsed to
  choose a lowering route. Only stable enum/status fields may gate admission.
- A proof certificate such as a local-array range proof is consumed directly by
  target code while bypassing the derived required fact it proves.
- A compatibility fact such as Route 6 source records, focus-function filters,
  or raw `PreparedBirModule*` access is introduced into a new target surface
  without either promotion into a typed required/feature view or an explicit
  migration owner.
- A missing required fact is handled by reconstructing semantic authority from
  raw BIR shape, diagnostic route names, or one narrow testcase pattern instead
  of failing closed.

These rules preserve the contract-first safety of the current prepared
pipeline while allowing the future view to shrink the MIR input surface.
