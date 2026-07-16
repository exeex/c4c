# Dependency Ordering and First-Owner Successor Scopes

Evidence revision: `da06fe48f1a8b534d29be14357b865035082cdf3`

This is an implementation queue design, not generated successor ideas or a
claim that any capability has been implemented.  “First owner” below means the
one proposed successor that changes the named semantic contract.  Existing
open ideas are dependencies/evidence only unless explicitly named as their own
existing owner; no scope below reopens 734, 759--763, 775/776, 795,
797, 801, 803, 811, 812/813, 814/815, 829--831, or parked 836.

## Ordering rule

1. **A1 canonical aggregate ref/store convergence** is first: later family,
   call, verifier, and producer work must have canonical aggregate identity to
   consume.  It consumes 754/801/832--836 evidence but does not complete 836;
   parked 836 still returns to parent 831 Step 4.
2. **F1 function signature and call composition** follows A1.  Its signature
   schema can refer to nominal family refs, while individual family producer
   migrations remain in their own first-owner scopes.
3. **V1, S1, U1, P1, C1, and D1** follow in dependency order: vector and
   scalar facts, then actual polymorphic boundaries, then direct producer and
   consumer transitions.  Each is independently buildable after the specific
   prerequisite listed below.
4. **T1 terminal universal-model deletion** is last.  It can start only when
   every prior adapter deletion gate is met.  Its accepted handoff feeds 797.
5. Only after the accepted type-family capabilities above may 812 refresh its
   string-authority inventory; 813 then owns the residual *non-type* string
   routes.  Neither umbrella is a substitute owner for a type-family row.

## Proposed successor contracts

| Order / first owner | Matrix rows owned exactly here | Dependencies and exact scope | Required proof | Adapter retirement / completion gate |
| --- | --- | --- | --- | --- |
| A1 — canonical module-owned aggregate ref/store convergence | M4, M5, M6 | Depends on the accepted 834/835 relation and records 801/832--836 evidence. Introduce `HirAggregateRef { ModuleId, HirAggregateId }` (or a proven-equivalent stable carrier) from every aggregate-bearing `QualType` to the canonical HIR aggregate store, then intern/map it once per LIR module into `LirAggregateRef` and an aggregate store. The map lifetime is the owning LIR module/lowering session and is never reusable across modules. Store struct/union kind, ordered fields, layout, projections, and recursive typed children. Cover named, anonymous, local, template, and typedef/alias occurrences. Register canonical HIR definitions before lowering any occurrence; resolve/intern before field/projection/call use. Unknown, incomplete, stale, foreign, or wrong-module refs fail closed—there is no lookup-by-tag recovery. | Fresh build plus focused lowering/verifier/printer cases for every aggregate form; nested field recursion; repeated occurrence interning; registration-before-use; foreign/wrong-module/incomplete rejection; and parity for the preserved 754/798/801/803 seams. Keep the three 836 groups separately asserted (incomplete key/ref, present-but-unmatched module owner, legitimate no-owner compatibility). | Retire parser `record_def`, reconstructed owner keys, tag spelling, rendered-text/cross-table lookup, and compatibility fallbacks only after every named declaration, field, call, verifier, printer, and receiver consumer resolves the canonical store ref. Delete duplicate owner, identity, layout, operation-result, and field metadata only after projections and consumers use store facts with parity. Legitimate no-owner compatibility remains explicitly named until its consumer migrates; it is not silently treated as an aggregate owner. |
| F1 — function-signature and call composition | M8, M9 | Depends on A1 for aggregate alternatives and preserves 761 plus the 829/830 body-use boundary. Add `LirFunctionSignatureRef`/store for return, ordered parameter family refs, variadic and ABI facts; make declarations and calls compose it. At actual call argument/result boundaries use only the later U1 bounded value carrier; do not absorb 829/830 body-use identity. | Fresh build plus declaration/call lowering, fixed/variadic signature verification, aggregate parameter/return composition, raw-call compatibility, printer, and reference-collection parity. Prove malformed signature and wrong-module aggregate alternative rejection. | Remove semantic `signature_text`, `args_str`, parsed-call construction, and duplicate `arg_type_refs` only after all named declarations, calls, verifier, printer, and reference collectors consume signature/value facts. Preserve raw extern/inline-asm output only behind named one-way adapters. |
| V1 — nominal vector store and vector-schema migration | M7 | Depends on A1 where the vector element is aggregate; preserves 754, 811, 814, and 815. Add vector store/ref with lane count and typed element family ref, then migrate vector operations and masks without making row-local shape data authority. | Fresh build plus vector lowering/verifier/printer and Insert/ExtractElement, shuffle/mask, poison second-shape, and malformed lane/element coherence cases. | Delete `LirNativeVectorShape`, mask text, and operation type-string duplicates only after all vector schemas read the vector store and equivalent malformed checks pass. |
| S1 — nominal scalar and ABI-leaf migration | M1, M2 | Depends only on retained 759/760 compatibility boundaries; introduce scalar store/ref and migrate scalar operation schemas, including separately evidenced pointer/void ABI leaves. Opaque semantics remain an evidence-needed subroute, not an assumed scalar. | Fresh build plus scalar lowering, scalar operation verifier/printer/receiver coverage, pointer/void ABI compatibility, and wrong-family rejection. | Remove scalar text classification/comparison and scalar `LirTypeRef` fields only after each named scalar op, verifier, printer, and receiver accepts `LirScalarRef`. Retire extern/inline-asm opaque factories only after their named native consumer exists. |
| U1 — restricted first-class value boundary unions | M10 | Depends on A1, V1, and S1 for admitted alternatives and preserves 775/776 and 754. Add only boundary-local enum-tagged value unions for call argument/result, PHI, select, and return, with C++20 traits and checked access. | Fresh build plus exhaustive enum handling at every named boundary, valid scalar/vector/aggregate (and separately evidenced pointer) alternatives, malformed/wrong-kind rejection, coercion, verifier, printer, and BIR receipt coverage. | Delete boundary `LirTypeRef` fields only after all named boundaries use exhaustive checked alternatives. No universal variant, generic ID, or implicit cross-family adapter is permitted. |
| P1 — direct HIR-to-LIR family construction and array composition | M3, M11 | Depends on A1/V1/S1/F1 and U1 at the explicit value boundary. Replace construction from `llvm_ty()`/rendering with producer-specific construction from `TypeSpec`, aggregate ref/layout, vector facts, and signature facts. Arrays retain typed element refs and length as a recursive composition contract. | Fresh build plus HIR lowering for scalar/vector/array/nested aggregate/signature producers, GEP typed-element verification, and proof that no printer output is reparsed for semantic state. | Delete each `runtime_text` or rendered aggregate/array factory only after its exact producer emits a family ref; retire array compatibility text after nested elements render from typed refs and GEP uses typed facts. Named parser/extern/asm adapters remain until their specific consumer migrates. |
| G1 — globals, extern declarations, and initializer type facts | M15 | Depends on S1/A1/V1/F1/P1; preserves 760 and 762 boundaries. Migrate global/extern type facts from parallel `TypeSpec`/text/optional-ref mirrors to family refs without taking ownership of initializer-text semantics. | Fresh build plus global and extern lowering, verifier/printer, family-ref collection, and legacy initializer compatibility cases. | Delete semantic `llvm_type` and extern runtime-text use only when global/extern consumers use family refs. Final output text and the legacy initializer scanner stay until the named producer/reference carrier migration proves their deletion. |
| R1 — typed reference carriers and collector migration | M14 | Depends on F1, P1, and G1; preserves 734 and coordinates (but does not duplicate) 812/813. Replace each raw-text symbol scan only with an exact producer-bound semantic callee/argument/signature/global reference carrier. | Fresh build plus call/global reference collector and LIR-to-BIR preparation proof for every replaced source field; verify no missing or spurious references against the compatibility route. | Delete a scanner only when its exact source field has a semantic reference carrier. Never reconstruct references from rendered text; residual non-type text is queued for 812 then 813. |
| C1 — family-overloaded verifier, semantic dispatch, and printer | M12, M13 | Depends on A1/F1/V1/S1/U1/P1/G1. Replace generic classification with `require_scalar`, `require_vector`, `require_aggregate`, `require_signature`, and value overloads; add matching one-way printer/dispatch overloads. Aggregate checks module/store and recursive children. | Fresh build plus focused valid/malformed/foreign/wrong-module/wrong-family verifier coverage, exhaustive dispatch coverage, and byte-for-byte rendering parity where output is contractual. | Delete generic `require_type_ref`, generic renderer, mutable semantic `.str()` use, and `LirTypeRef(type.str())` classification only once every named caller uses an overload and compatibility mirrors are merely checked, never parsed. |
| T1 — terminal universal `LirTypeRef` and string escape-hatch deletion | M16 | Depends on deletion acceptance from A1, F1, V1, S1, U1, P1, G1, R1, and C1. Remove the universal bag/API only after every M1--M15 responsibility has its named replacement; hand the complete valid-LIR disposition to 797. | Fresh full build and broad regression/coverage proof, including compile-time separation, no remaining universal fields/factories/conversions, and receiver/dispatcher proof required by 797. | Remove mutable `str()`, implicit string conversions, textual equality/classification, semantic `runtime_text`, and expired adapters only when no consumer remains. This is terminal; it cannot retain a universal compatibility substitute. |

## Exact row-assignment check

| Matrix row | One first owner |
| --- | --- |
| M1 | S1 |
| M2 | S1 |
| M3 | P1 |
| M4 | A1 |
| M5 | A1 |
| M6 | A1 |
| M7 | V1 |
| M8 | F1 |
| M9 | F1 |
| M10 | U1 |
| M11 | P1 |
| M12 | C1 |
| M13 | C1 |
| M14 | R1 |
| M15 | G1 |
| M16 | T1 |

Each M1--M16 row appears once in this check and once in the successor contract
table.  Scope dependencies do not transfer first ownership or create duplicate
open ownership; Step 4 must create at most one source idea for each proposed
scope and retain existing open ideas strictly as named dependencies.
