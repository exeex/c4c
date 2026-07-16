# Historical Blocker-to-Root-Model Map

Evidence revision: `ac4af813dd27f29a1bbe48aeb28e67ed1eebec11`

Each row maps a required 837 historical wall to the overloaded responsibility
it exposed.  “No new owner” means a closed capability is evidence/dependency,
not permission to reopen or duplicate it.

| Evidence wall | Root responsibility exposed | Accepted capability/preservation rule | Current owner status |
| --- | --- | --- | --- |
| 759 | builtin identity was coupled to spelling | preserve enum-first builtin identity and rendered output boundary | closed; no new owner |
| 760 | constructors admitted semantic runtime text | preserve only named parsed-call, extern, inline-asm, aggregate/field/signature, GEP, padding, vector-ABI, and va-list compatibility boundaries | closed; remaining boundaries are evidence for later deletion owners |
| 761 | call/signature and switch mirrors competed with raw text | preserve structured precedence, raw-call compatibility, and intentional inline-asm exclusion | closed; function/call successor depends on it |
| 763 | composite forms were isolated in a universal bag | preserve builtin/integer, named struct/union, and array islands; do not claim vector, aggregate literal, function, recursive, or runtime-text closure | closed; no new owner |
| 775/776 | PHI producer and expression/coercion APIs lost result/type authority | preserve typed result carrier/decomposition without folding every expression into a type-family repair | closed; producer dependencies remain separate |
| 754 | aggregate/vector operation rows used local type/result facts | preserve ExtractValue, InsertValue, InsertElement, ExtractElement, and ShuffleVector capability; do not call it generic family completion | closed; aggregate/vector successors consume evidence |
| 798 | aggregate operand provenance was missing | preserve selected native operand provenance and exclusions | closed; no new owner |
| 801 | anonymous aggregate layout was text-primary | preserve native anonymous field/layout facts and structured call/layout handoff | closed; aggregate successor dependency |
| 803 | selected aggregate SSA producers lacked authority | preserve local-load and constructed-insertvalue authority only | closed; no new owner |
| 811 | vector result/use, lane, element, index, and mask were scattered | preserve reusable native carrier; do not reinterpret it as generic vector closure | closed; vector successor dependency |
| 814/815 | poison second-shape and mask lane coherence were split | preserve bounded splat seam and fail-closed malformed checks | closed; do not reopen/generalize |
| 762 | module declaration/type shadows competed with text | preserve structured declaration/type-shadow precedence; exclude full type tree, HIR rewrite, and global initializer semantics | closed; module facts are dependency evidence |
| 795 | body parameters lacked selected native handoff | preserve bounded selected route; ABI/byval/HFA/vector/variadic remain uncompleted | open owner, bounded scope |
| 829/830 | direct-call argument identity/type relation and call signature/body-use were conflated | preserve ordered 829 -> 830 chain and distinct ownership of signature facts versus body-use identity | open dependencies; do not duplicate |
| 832 | aggregate function parameter crashed without owner relation | preserve bounded crash repair | closed; no new owner |
| 833 | truthiness LHS parameter required direct scalar authority | preserve direct-scalar bounded repair | closed; no new owner |
| 834/835 | durable aggregate owner identity and selected module canonicalization were absent | preserve durable `QualType` identity and selected `lir_owned_type_spec` relation, while rejecting parser pointer lifetime dependence | closed prerequisites; not a full occurrence-to-definition contract |
| 836 structured-key cases 50,52,510,661,688,692,3038 | aggregate occurrence has no complete stable owner key before LIR-owned lowering | preserve fail-closed rejection; no rendered/tag fallback for incomplete structured identity | parked at Step 1; fresh decomposition required |
| 836 matching-owner cases 201,202,217,1529 | key-to-canonical-module-definition resolution fails | preserve matching-module-owner rejection; no text-derived owner substitution | parked at Step 1; separate first owner required |
| 836 no-owner case 46 | legacy rendered compatibility is a distinct no-owner contract | preserve compatibility only after structured misses are fenced; do not weaken structured diagnostics | parked at Step 1; separate contract required |
| 734 | receiver needed a typed LIR handoff | preserve receipt/import boundary; never assign missing producer/type model repair to it | open dependency after producer handoff |
| 797 | final dispatcher/proof needs every valid fact disposed | preserve terminal integration role; not a catch-all architecture owner | open terminal dependency |
| 812/813 | broader string inventory/routing could stale-duplicate type work | preserve their row keys and run 812 refresh then 813 only after accepted type-family capabilities | open dependencies; no duplication |

## Root-model conclusion

The repeated wall is not one generic “aggregate type failure.”  The current
model asks one `LirTypeRef` plus scattered mirrors to carry nominal identity,
recursive layout, vector shape, function composition, compatibility text, and
rendering.  In HIR it additionally asks occurrences to recreate their way back
to the canonical definition through `HirRecordOwnerKey` and module lookup.

The required future contract is narrower and testable: a canonical,
module-owned aggregate definition/store owns identity, fields, layout and
projections; every aggregate-bearing occurrence carries a stable canonical
reference; nested children retain typed references; and foreign, wrong-module,
incomplete, and legitimate no-owner cases remain distinguishable.  Step 1
does not select an implementation, create a successor, or repair any row.

## Resumption boundary

836 retains its exact Step 1 return point.  Once its bounded route is accepted
and proven, the required parent return is 831 Step 4’s comparable full-suite
gate, not 830 or 829.  If 837’s later priority successors change the facts,
plan ownership must reclassify using fresh evidence rather than declare 836
complete or superseded.
