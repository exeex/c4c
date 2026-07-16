# LIR String Authority Current-State Inventory Umbrella

Status: Open
Type: Umbrella evidence classifier and disposition-map producer
Parent: none
Handoff Directory: `docs/lir_string_authority_remaining_routes/`
Downstream Router: `ideas/open/813_lir_string_semantic_authority_completion_umbrella.md`
Related:
- `ideas/closed/759_lir_typed_ref_enum_foundation.md`
- `ideas/closed/760_lir_string_constructor_deprecation_migration.md`
- `ideas/closed/761_lir_call_signature_type_mirror_convergence.md`
- `ideas/closed/763_lir_composite_type_ref_model.md`
- `ideas/closed/754_lir_aggregate_vector_value_identity_convergence.md`
- `ideas/closed/762_lir_module_declaration_type_shadow_convergence.md`
- `ideas/closed/811_lir_native_vector_authority_carrier_publication.md`
- `ideas/closed/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`
- `ideas/closed/815_lir_shuffle_vector_native_mask_lane_coherence_repair.md`
- `ideas/closed/832_hir_aggregate_owner_function_parameter_crash_repair.md`
- `ideas/closed/833_lir_truthiness_lhs_parameter_authority_completion.md`
- `ideas/closed/834_lir_owned_type_spec_module_owner_canonicalization_blocker.md`
- `ideas/closed/835_hir_durable_aggregate_owner_identity_carrier_prerequisite.md`

## Goal

Build the authoritative current-state inventory of every LIR field, API, and
callsite where `std::string`, rendered text, or text-derived classification may
still influence semantic identity, type, ownership, lookup, verification, or
dispatch. Assign every row an evidence-backed disposition and exact current
owner status so 813 can route only genuinely unfinished, unowned work.

This idea owns evidence reconciliation and mapping. It does not generate
successor ideas or implement any semantic repair.

## Why This Exists

The original string-authority inventory predates substantial accepted work.
Closed 754 now completes its five representative aggregate/vector operation
rows; closed 762 establishes structured module declaration/type-shadow
precedence; and closed 811/814/815 establish the bounded vector carrier,
poison-second-shape, and mask-lane contracts. Closed 832-835 establish bounded
aggregate-owner lifetime and canonicalization relations. Treating those routes
as still wholly open would duplicate accepted work.

Their closures are also bounded. They do not prove universal recursive type
shape closure, every aggregate owner path, every body parameter, every
instruction producer, or retirement of generic runtime-text APIs. Current code
still contains searchable semantic-risk surfaces such as `LirTypeRef::runtime_text`,
text-derived type classification, mutable/implicit string access, parsed raw
call helpers, expression/coercion string returns, global initializer/reference
text scans, and compatibility parsing. A current producer-to-consumer matrix is
required before lifecycle routing can be honest.

## Current Evidence

Execution must take a fresh tree revision and record it as the baseline. At
minimum it must inspect:

- closure records for 759-763, 754, 811, 814, 815, 832, 833, 834, and 835;
- current open contracts and resumption records for 734, 795, 796, 797,
  829-831, and active 836, plus any newer owner present at execution time;
- `src/codegen/lir/types.hpp`, `operands.hpp`, `ir.hpp`, `call_args.hpp`,
  `call_args_ops.hpp`, HIR-to-LIR producers, `verify.cpp`, `lir_printer.cpp`,
  reference collectors, and LIR-to-BIR consumers;
- every named runtime-text factory, mutable or implicit string conversion,
  text classifier/parser, raw operand/result seam, rendered mirror, and
  text-based reference scan.

Newer code and accepted closure evidence supersede stale draft examples and
counts. A closure satisfies only the exact family its completion record names.

## Required Dispositions

Every matrix row must receive exactly one disposition:

- `structured-authoritative`: native IDs/enums/refs/nodes are semantic
  authority and no semantic consumer depends on text;
- `checked mirror`: text is retained for output/compatibility, is derived from
  or checked against structured authority, and cannot override it;
- `intentional opaque/render text`: the payload is intrinsically textual or a
  one-way final rendering boundary and is forbidden as semantic input;
- `existing open owner`: unfinished work is explicitly owned by a named open
  idea, with exact scope, dependency, and return point;
- `unowned semantic route`: current evidence proves unfinished semantic text
  authority and no open source owns it; this becomes 813 input;
- `insufficient evidence`: the producer/consumer relation is not yet proven;
  the missing evidence and first diagnostic action must be explicit.

`legacy`, `runtime text`, `compatibility`, and `std::string` alone are not valid
dispositions.

## Required Matrix

Create `docs/lir_string_authority_remaining_routes/field_callsite_disposition_matrix.md`.
Each row must include:

| Column | Required meaning |
| --- | --- |
| Field/API/callsite | Exact declaration and semantic-use location. |
| Producer | Where the value or text originates. |
| Semantic consumers | Verifier, lookup, dispatch, lowering, reference collection, or receiver behavior affected. |
| Structured authority | Existing ID/enum/ref/node, or the exact missing native fact. |
| Text precedence | Whether text can override, repair, classify, or only render/check. |
| Disposition | Exactly one allowed disposition above. |
| Closure evidence | Exact bounded closure that satisfies the row, or why it does not. |
| Open owner | Exact current source scope/return point, or `none`. |
| Required proof | Static, malformed-mirror, producer/consumer, or behavioral evidence. |
| 813 routing key | Stable row/family key for downstream routing. |

For every `LirTypeRef`-bearing row, recursively record the outer and nested
shapes and the first level that remains text-primary. An outer structured
vector, aggregate, array, signature, or module field is not proof that nested
element, field, return, parameter, or child refs are structured.

## Required Evidence Documents

Create under `docs/lir_string_authority_remaining_routes/`:

- `evidence_baseline.md`: tree revision, scanned surfaces, closure inventory,
  and current open-owner inventory;
- `field_callsite_disposition_matrix.md`: canonical exhaustive matrix;
- `closure_reconciliation.md`: satisfied, bounded-only, superseded, and still
  relevant closure-note claims;
- `existing_owner_map.md`: exact open scope and return points;
- `intentional_text_registry.md`: intrinsic text, rationale, and forbidden
  semantic uses;
- `handoff_to_813.md`: only unowned/insufficient rows and stable routing keys;
- `closure_trace.md`: counts, omissions, and final evidence revision.

## Required Classification Families

The inventory must cover at least:

1. `LirTypeRef` constructors, `runtime_text`, classification, mutable `str()`,
   implicit conversions, comparisons, and recursively nested type forms.
2. Parsed/raw call arguments, return types, callee suffixes, `args_str`, raw
   compatibility parsing, and semantic reference collection. Closed 761 is
   authoritative only for its completed structured mirror precedence.
3. Module/global/extern/function/struct shadows and reference scans. Closed 762
   satisfies its named declaration/type-shadow rows, but its explicit
   out-of-scope global initializer semantics and full type-tree work remain
   independently classified.
4. Global linkage/visibility/qualifier policy, initializer text/tree facts,
   symbol and function-reference identity, and string-pool/global/name identity.
5. Expression/result/coercion APIs, `LirOperand::raw`, `fresh_tmp` presentation,
   and producer paths that may lose value/type/owner authority.
6. Body parameters, direct-call arguments, and ABI-expanded forms. Map the
   bounded 795 acceptance and current 829-831/836 chain exactly; do not infer
   family-wide completion.
7. Residual instruction, intrinsic, terminator, CFG/PHI, and inline-assembly
   value/type bindings. Map bounded 796 work; keep templates/constraints opaque.
8. Aggregate/vector authority. Mark closed 754/811/814/815 rows satisfied,
   while separately classifying recursive type shapes and aggregate owner
   routes beyond those contracts. Map active 836 rather than reopening 832-835.
9. Intentional text: inline-assembly templates/constraints, raw literal bytes,
   data layout, diagnostics, final LLVM rendering, and display/mangled names
   whose semantic identity is separately carried.
10. LIR-to-new-BIR receiver/final proof dependencies: map 734 receipts and 797
    integration without assigning producer repair to either by default.

## In Scope

- Read-only evidence collection and documentation under the handoff directory.
- Exact field/callsite tracing from producer through semantic consumers.
- Reconciliation of bounded closures and current open owners.
- An exhaustive disposition matrix and stable handoff to 813.
- Explicit identification of unowned and insufficient-evidence rows.

## Out Of Scope

- Creating successor ideas; 813 owns later generation and ordering.
- Implementing schema, producer, verifier, printer, receiver, or test changes.
- Activating, switching, repairing, or closing active/open lifecycle work.
- Treating every string as debt or requiring IDs for intrinsic text.
- Parsing text, names, operands, diagnostics, printer output, or testcases to
  reconstruct semantic authority.
- Claiming semantic capability completion from inventory/classification.

## Dependency And Ordering

812 may be activated only without displacing a higher-priority active route.
Its evidence baseline must reflect the state at activation, not this promotion
snapshot. 812 completes before 813 executes. It must not create successors or
reserve future idea numbers.

Existing owners retain their lifecycle order. In particular, active 836 owns
the demonstrated residual aggregate-owner decomposition and returns through
831 to 830 and 829; 812 records that chain but does not alter it. Closed 754,
762, 811, 814, 815, and 832-835 are evidence, not activation candidates.

## Acceptance Criteria

- All required evidence documents exist and identify one current tree revision.
- Every relevant closure-note deferred/out-of-scope claim maps to a matrix row,
  a bounded satisfied row, an intentional-text row, an existing owner, an
  unowned route, or explicit insufficient evidence.
- Every matrix row traces producer, semantic consumers, structured authority,
  text precedence, exact disposition, proof, and a stable 813 routing key.
- Recursive `LirTypeRef` shape evidence distinguishes structured outer carriers
  from nested text-primary forms.
- Closed 754/762/811/814/815 and 832-835 are credited only for their exact
  accepted scopes; no completed row is routed again and no bounded closure is
  generalized to a whole family.
- Current 734/795/796/797/829-831/836 and newer owners are mapped without scope
  expansion or duplicate ownership.
- Every intentional text row states why text is intrinsic and which parsing,
  lookup, verification, or dispatch uses are forbidden.
- `handoff_to_813.md` contains all and only unowned/insufficient rows; it does
  not create or prescribe successor idea numbers.
- No implementation, test, expectation, runtime, active-plan, `plan.md`, or
  `todo.md` change occurs.

Completion is evidence-classification capability only. It must not be described
as completing any semantic authority migration; durable unfinished rows remain
open through existing owners or 813 routing.

## Closure Note Requirements

The closure note must name the evidence revision, documents, matrix row counts
by disposition, every bounded closure credited, every current open owner and
return chain, every unowned/insufficient row handed to 813, and every excluded
intentional text family. It must state explicitly that no successor or semantic
implementation was created.

## Reviewer Reject Signals

- Reject implementation, test, verifier, schema, runtime, receiver, lifecycle,
  or successor-creation changes inside 812.
- Reject filename/count-only inventory without producer and semantic-consumer
  tracing.
- Reject every-string-is-debt reasoning or IDs/enums for intrinsic text.
- Reject `legacy compatibility` or `runtime text` as an unexplained terminal
  disposition.
- Reject treating closed 754/762/811/814/815 or 832-835 as broader than their
  closure records, or rerouting their completed rows.
- Reject duplicate ownership against 734/795/796/797/829-831/836 or newer open
  sources.
- Reject semantic facts recovered from rendered text, names, operands, printer
  output, diagnostics, or testcase identity.
- Reject weaker verifier/tests, expectation downgrades, unsupported markers,
  allowlist filtering, or named-case evidence.
- Reject claiming classification as implementation or semantic completion.
- Reject closure while any discovered row lacks a disposition, stable 813 key,
  or explicit missing-evidence statement.
