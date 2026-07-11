# Old BIR And New BIR Equivalence Strategy

This document answers Step 3's producer-equivalence question: how the current
BIR/prealloc pipeline and a future rewritten BIR pipeline can both produce the
same `PreparedMirView` contract while MIR targets continue to lower through
one stable boundary.

The design claim from the source idea remains:

```text
Old BIR -> current prepare ------\
                                  -> PreparedMirView -> MIR
New BIR -> new/compat prepare ---/
```

Equivalence must be defined at the view boundary, not by requiring the new
producer to clone every field, note, route name, or proof artifact in today's
`PreparedBirModule`.

## Producer Architecture

The old producer is the current semantic-BIR preparation path:

- `src/backend/backend.cpp:300` calls
  `prepare::prepare_semantic_bir_module_with_options()`;
- `src/backend/prealloc/prealloc.cpp:31` runs legalize, stack layout,
  liveness, out-of-SSA, regalloc, and `publish_contract_plans()`;
- `publish_contract_plans()` publishes frame, dynamic stack, call,
  publication, local-array proof/provenance, variadic, storage, object-data,
  special-carrier, atomic, intrinsic, inline-asm, and runtime-helper families;
- MIR entry points such as x86 `emit_prepared_module()` currently receive the
  resulting `PreparedBirModule` directly, as described in
  `01_current_mir_dependencies_on_prepared_bir.md`.

The first old-route adapter should be:

```text
bir::Module + TargetProfile
  -> prepare::PreparedBirModule
  -> PreparedMirViewAdapter
  -> MIR target
```

The future new producer should target the same adapter-facing contract:

```text
new BIR / new lowering IR + TargetProfile
  -> NewPreparedMirFacts
  -> PreparedMirViewAdapter
  -> MIR target
```

MIR targets should not know which producer built the view. Producer identity
may appear in a diagnostic-only field such as `producer_kind()` or
`diagnostics().producer_route()`, but no lowering API should branch on it.

## Boundary Equivalence Definition

Two producers are equivalent for MIR when their `PreparedMirView` projections
are structurally equal for the admitted target, output mode, function set, and
feature set.

Equivalence includes:

- target identity: same target arch/profile and target triple as exposed by
  `target_profile()` and `target_triple()`;
- module identity: same defined-function set, declarations policy, globals,
  string constants, public names, and link names as exposed by the core view;
- durable ids: same `FunctionNameId`, `BlockLabelId`, `ValueNameId`,
  `PreparedValueId`, slot/object/link ids, or a deterministic bijection that
  is resolved before comparison and invisible to MIR;
- traversal binding: every exposed instruction cursor points to the same
  semantic operation, function, block, block index, instruction index, and
  prepared control-flow block;
- core facts: equal control-flow blocks, branch targets, branch conditions,
  join transfers, parallel-copy bundles, value homes, stack layout,
  addressing/provenance records, and view-owned lookup answers;
- optional feature facts: for every admitted feature, equal call plans,
  variadic helper homes, i128/f128 carriers, atomic/intrinsic/inline-asm
  carriers, object-data records, and runtime-helper boundaries;
- publication and freshness authority: equal source and destination ids,
  source homes, destination homes, block-entry/edge move records, source
  freshness answers, and fail-closed statuses;
- verifier outcome: equal stable verifier `fact_family`, owner class,
  `fail_closed` bit, status enum, ids, and feature-presence decisions.

Equivalence excludes:

- note order and note text;
- `completed_phases`;
- private producer `route_`;
- debug summary/trace text;
- focus-filter counts and display-only spelling;
- verifier `detail` wording when stable status enums and ids match;
- proof-certificate internal shape when both producers derive the same
  required fact and the verifier accepts it.

If a fact is target-local by design, such as final opcode spelling,
scratch-register choice, object relocation encoding, or text-assembly layout,
it should not be compared as a producer-equivalence fact unless it is exposed
through a named feature view.

## Comparison Layers

The comparison strategy should be layered so textual drift does not hide
structural divergence and structural comparison does not require freezing the
entire old prepared module.

| Layer | Purpose | Required checks |
| --- | --- | --- |
| View schema version | Ensure both producers claim the same contract. | `PreparedMirView` version, target, output mode, enabled feature views, and required optional-view presence. |
| Golden canonical dump | Human-readable regression aid. | A deterministic dump of only view-exposed core and feature facts, sorted by ids/cursors, without notes, phase strings, private route names, or debug summaries. |
| Structural comparison | Main equivalence proof. | Typed comparison of target identity, names, functions, cursors, control flow, value locations, stack layout, addressing, lookups, publications, and optional feature views. |
| Verifier comparison | Admission and fail-closed proof. | Run the same verifier suite over both projections and compare stable report fields: fact family, owner class, status, `fail_closed`, function/value/cursor ids, and missing-feature category. |
| Target dry-run admission | Optional guard before codegen. | Ask each MIR target whether the view is admitted, and compare admission/unsupported outcomes without emitting instructions. |
| MIR or machine snapshot | Late confidence check. | Compare target MIR or object model only after view equivalence is green. This must not replace structural view comparison. |

Golden dumps should be generated from `PreparedMirView`, not from
`prepare::print(PreparedBirModule)`. The current prepared printer renders
route, completed phases, invariants, and notes in
`src/backend/prealloc/prepared_printer.cpp:42`; those are useful diagnostics
but are not part of producer equivalence.

## Canonical Dump Shape

A canonical dump should be intentionally boring and parser-friendly:

```text
prepared_mir_view version=1 target=x86_64 triple=...
function id=... name=...
  block index=... label=...
    inst index=... cursor=...
    value_home value_id=... kind=... slot=... reg=...
    addressing cursor=... base=... offset=... width=... provenance=...
    publication pred=... succ=... dst=... src=... move=...
  feature calls present=true
    call cursor=... callee=... args=... result=...
verifier fact_family=... owner=... fail_closed=false cursor=...
```

The dump should omit:

- old prepared route names such as `SemanticBirShared`;
- preparation phase history;
- raw `PrepareNote` messages;
- x86 route summary lines such as `route owner` or grouped counters;
- Route 6 debug gate text;
- target-local instruction text produced after MIR admission.

The dump can include stable enum names for required facts and verifier
statuses, but those names must be generated from the view schema rather than
copied from diagnostic message text.

## Structural Comparator

The structural comparator should operate on typed snapshots or direct view
accessors:

```cpp
struct PreparedMirViewEquivalenceOptions {
  TargetProfile target_profile;
  OutputMode output_mode;
  std::span<const PreparedMirFeatureKind> required_features;
  bool compare_diagnostic_view = false;
};

PreparedMirEquivalenceReport compare_prepared_mir_views(
    const PreparedMirView& old_view,
    const PreparedMirView& new_view,
    PreparedMirViewEquivalenceOptions options);
```

The report should classify differences by contract severity:

- `BlockingRequiredFactDifference`: ids, cursor binding, control flow, value
  homes, stack layout, addressing, publication, or feature fact differs;
- `BlockingPresenceDifference`: one view has a required feature view and the
  other does not;
- `BlockingVerifierDifference`: stable verifier status or `fail_closed`
  differs;
- `NonBlockingDiagnosticDifference`: note text, phase text, route trace, or
  diagnostic wording differs;
- `ComparatorUnsupported`: the comparator lacks support for a feature and
  must fall back to old BIR for that unit.

The comparator must be structural before textual. A textual dump match is
supporting evidence only.

## Verifier Checks

The existing verifier surface already points to the right shape:
`PreparedContractVerificationReport` records a fact family, owner class,
function/value ids, `fail_closed`, and detail text. Step 3 should preserve
stable fields as comparison keys and keep detail text out of lowering
authority.

Verifier comparison should include at least:

- value-home typed storage and value materialization;
- call-boundary argument/result plans;
- call-argument typed routes and raw ABI coherence;
- variadic-entry helper homes;
- selected local storage;
- memory access provenance;
- selected object data;
- frame-slot address and value source routes;
- local-frame address materialization source routes;
- binary-producer materialization facts for call arguments;
- feature-specific completeness for special carriers, atomics, intrinsics,
  inline asm, and runtime helper boundaries.

For local-array proof families, the verifier should compare the required
derived facts first, such as local address provenance and scalar local loads.
It may also compare proof certificates, but proof-certificate differences are
blocking only when they change derived required facts or verifier acceptance.

## Compatibility Fields To Exclude

The new contract must not freeze these current fields as permanent
`PreparedMirView` ABI:

| Current field or path | Why it should be excluded |
| --- | --- |
| raw `PreparedBirModule::module` | The core view should expose functions, BIR traversal handles, globals, string constants, and name tables through narrow read-only accessors. It should not expose whole-module ownership. |
| `invariants` vector | Successful invariant verification is a precondition/verifier outcome. MIR should not consume the raw vector as semantic input. |
| `liveness` | Not part of the smallest required view in Step 2. Promote only through a named feature view if a target proves it needs direct liveness facts. |
| `register_group_overrides` | Currently diagnostic or allocation-detail state. Do not make it core. |
| `completed_phases` and `notes` | Diagnostic history. Backend comments at `src/backend/backend.cpp:805` already describe prepare notes as rendered diagnostics. |
| private `route_` and `prepare_route()` | Producer provenance. It can appear in diagnostics but must not influence MIR lowering. |
| x86 route summary/trace text | Observational debug output from `src/backend/mir/x86/debug/debug.cpp:425`, not structural contract. |
| backend route-debug focus filters | Public dump filters over rendered names, described as non-semantic in `src/backend/backend.cpp:306`. |
| Route 6 debug gate names | Useful proof/status text, but the view should expose stable source-dependency facts or verifier statuses instead. |
| compatibility projections such as AArch64 `machine_nodes` | Target-local output compatibility, not producer input equivalence. |
| raw `PreparedBirModule*` in target contexts | Migration holdout that can bypass the view boundary. It must be removed from target code rather than copied into the new contract. |

Current optional feature fields such as call plans, object data, carriers,
atomics, intrinsics, inline asm, and runtime helpers are not excluded
entirely; they should be exposed only through the optional feature views
defined by `03_feature_views_and_optional_contracts.md`.

## Fallback To Old BIR

Fallback must happen before MIR target lowering observes an incomplete new
view. The new producer should be treated as an attempted producer for one
module/function/feature unit, not as an authority that can partially feed MIR.

Recommended fallback flow:

1. Build the old view from the current prepared route.
2. Attempt to build the new view for the same target and output mode.
3. Run schema, structural, and verifier comparison for the required core and
   admitted feature views.
4. If the new view is missing any required fact, cannot express a required
   optional feature, has a blocking verifier difference, or the comparator
   returns `ComparatorUnsupported`, discard the new view for that unit.
5. Lower through the old view and emit a diagnostic-only fallback reason.
6. Never merge old and new facts for one function unless an explicit bridging
   adapter defines ownership of each fact family and the structural comparator
   checks the mixed projection.

Fallback diagnostics should identify producer, function, feature, and stable
missing/different fact family. They should not be parsed by targets and should
not change emitted code.

## Acceptance Rules For Parallel Migration

A new producer slice is ready to route through `PreparedMirView` only when:

- the old-route adapter and new-route adapter both implement the same view
  schema version for the target subset;
- canonical dumps are deterministic and omit compatibility-only fields;
- structural comparison is green for the target/function/feature subset being
  enabled;
- verifier comparison is green or differences are classified as
  nonblocking diagnostic wording;
- fallback to old BIR is available and happens before MIR lowering;
- no MIR target code branches on producer kind, route name, note text,
  completed phases, or debug summaries.

This gives the future rewrite a stable output target without making the
current oversized `PreparedBirModule` permanent.
