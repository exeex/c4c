# Prepared Diagnostics And Oracle Replacement Checklist

Status: planning artifact for idea 194.

This checklist records the diagnostic and oracle prerequisites that must exist
before any prepared printer, debug, dump, route-debug, wrapper, or oracle-test
surface can contract. It does not claim prepared API contraction readiness.
Prepared surfaces remain compatibility oracles until every consumed semantic
fact has route-native diagnostics and equivalent oracle coverage.

## Readiness Rule

A retained prepared surface is not retirement-ready until all of these are true:

- The route-native replacement names the semantic route facts it owns.
- Existing prepared expected output and oracle strength remain intact.
- Positive, negative, ambiguous, mismatch, and fallback coverage exists for
  the affected route families.
- Any compatibility adapter still reading prepared facts is named and retained.
- Target policy, formatting policy, ABI policy, storage/home selection, and
  emission spelling remain outside route-native diagnostic ownership unless a
  separate implementation plan explicitly proves that ownership change.

Production lowering success, full-suite greenness, matching section names, or
printer label rewrites are not readiness evidence by themselves.

## Blockers By Surface

| Surface | Blocking consumers | Route families | Required replacement evidence |
| --- | --- | --- | --- |
| Prepared printer | `prepare::print(const PreparedBirModule&)` and prepared-printer tests. | Routes 1-7. | Route-native printer oracles that assert route facts separately from prepared labels, formatting sections, target policy, and compatibility output. |
| Prepared CLI dump | `--dump-prepared-bir`, prepared dump snippets, and `backend_cli_dump_prepared_bir_*`. | Routes 1-7, as exposed by dump output. | A separate route-native dump stage with coverage for positive selection, rejected prepared-stage aliasing, ambiguous combined flags or focus filters, mismatched route/prepared facts, and compatibility fallback to the prepared dump. |
| x86 route-debug summary/trace | `summarize_prepared_module_routes(...)`, `trace_prepared_module_routes(...)`, and x86 MIR summary/trace tests. | Route 6, Route 7, memory/return, address/source, value-home, edge and publication rows where summarized. | x86 route-native summary/trace rows sourced from route views or target-local lowering state, with compatibility-derived rows labeled until route-native authority owns every assertion. |
| Prepared lookup helper oracle | `backend_prepared_lookup_helper` and helper-status assertions. | Routes 1-7 plus call plans, preserved values, move bundles, frame-address offsets, return-adjacent facts, and aggregate fallback status. | Route-native helper-oracle families that reproduce both success and failure/fallback semantics without using prepared results as the source of truth. |
| AArch64 lookup threading | `PreparedFunctionLookups` passed through `FunctionLoweringContext` and AArch64 route-consuming tests. | Routes 1-7, return chains, target-local homes, storage, move facts. | Target-local route-view or context-construction coverage proving receipt and consumption of route-native facts independently from prepared lookup handles. |
| Target-wrapper compatibility | x86 wrapper tests, RISC-V prepared edge-publication tests, and wrapper inputs built by prepared lookup mutation. | Routes 4-6 first, then any routed wrapper fact reused by a target. | Wrapper-input coverage built from route-native views, while prepared wrapper assertions remain for adapters that still consume prepared lookups. |

## Blockers By Route Family

| Route family | Facts that must be covered before contraction | Compatibility blockers |
| --- | --- | --- |
| Route 1 producer and constant | Producer identity, integer constants, value origins, value homes, absent producer facts, ambiguous homes/producers, mismatch against prepared rows. | Prepared same-block producer, constant, scalar operand, and value-publication helpers remain fallbacks. |
| Route 2 select-chain and direct-global | Select roots, root instruction indexes, scalar eligibility, direct-global dependency presence, non-select paths, overlapping select/global candidates, mismatches. | Prepared select-chain materialization and direct-global dependency helpers remain fallbacks for publication, call, memory, ALU, FP, producer, edge-copy, printer, and oracle paths. |
| Route 3 memory and source | Memory access identity, address/source facts, store-source and load-source facts, non-memory negatives, alias/address ambiguity, prepared-vs-route mismatches. | Prepared memory access, global-load, same-block global-load, load-local source, and target-addressing policy helpers remain fallbacks. |
| Route 4 publication | Current-block and block-entry publication facts, missing publication, block-boundary ambiguity, publication-row mismatches. | Prepared current/block-entry publication, edge-publication lookup surfaces, x86 wrappers, and route-oracle tests remain public. |
| Route 5 edge and join-source | Edge facts, join-source facts, transfer facts, straight-line negatives, multi-predecessor ambiguity, prepared edge-row mismatches. | Prepared edge-publication, source-producer, join parallel-copy, and move-bundle helpers remain fallbacks. |
| Route 6 call boundary | Call-use facts, argument/result source facts, preserved values, returns, non-call negatives, reused-call or return-chain ambiguity, call-plan mismatches. | Prepared call plans, call argument/result plans, publication routing, value-home lookup, move bundles, and call-boundary effect plans remain public. |
| Route 7 comparison and condition | Comparison facts, fused-compare facts, branch-condition provenance, materialized-condition facts, non-branch negatives, compare/branch ownership ambiguity. | Prepared comparison helpers, scalar producer/select-chain fallbacks, fused-compare oracles, and target branch policy remain visible. |

## Blockers By Consumer Group

| Consumer group | Readiness blocker | Split guidance |
| --- | --- | --- |
| Diagnostic printers | They currently preserve compatibility labels and semantic facts for prepared route rows. | Split route-native printer work from prepared printer preservation. Do not merge with production lowering changes. |
| CLI dump routing | Prepared dump flags are compatibility surfaces, not route-native stages. | Scope route-native dump work to flag routing, focus behavior, mismatch reporting, and fallback behavior. |
| Route-debug summaries | x86 summaries and traces may expose compatibility-derived route rows. | Scope route-debug work per target and per summary/trace surface. Keep compatibility-derived rows labeled until replaced. |
| Oracle tests | Existing oracle tests protect both positive semantics and failure/fallback status. | Add route-native oracle tests beside prepared tests. Do not rewrite prepared expectations as replacement proof. |
| Target wrappers | Wrappers often build semantic cases through prepared lookup mutation. | Add wrapper-input construction from route-native views only after the route view is proven elsewhere. |
| AArch64 context threading | AArch64 receives the aggregate prepared lookup bundle as pass context. | Scope context-threading tests to one route fact family at a time and keep target-owned policy separate. |

## Compatibility Adapter Needs

- Prepared-to-route comparison adapters may be needed to show that route-native
  facts agree with prepared compatibility rows during migration.
- Route-native diagnostic dump adapters must keep prepared dump output and
  route-native dump output isolated so expected text remains independently
  reviewable.
- x86 route-debug adapters must distinguish compatibility-derived rows from
  route-native authority rows until the route-native source owns every asserted
  semantic fact.
- Helper-oracle adapters must preserve prepared success, missing-input,
  ambiguous-input, mismatch, and fallback status semantics while route-native
  helper families are introduced.
- Target-wrapper adapters must allow prepared lookup inputs and route-native
  inputs to coexist without treating prepared mutation as route-native proof.
- AArch64 context adapters must name whether each assertion used a prepared
  lookup handle, a route-native view, or an intentional fallback.

Adapters are retained compatibility surfaces, not contraction evidence. An
adapter can be removed only after the route-native surface proves the same
semantic and fallback contract without reading prepared data as authority.

## Retained-Oracle Rationales

- Prepared printer output remains because it is the compatibility oracle for
  route facts, prepared labels, and formatting while route-native diagnostics
  are incomplete.
- Prepared CLI dump output remains because existing dump snippets protect a
  user-facing diagnostic stage and must not be relabeled into route-native
  coverage.
- x86 route-debug remains because summaries/traces protect target-observable
  route rows and fallback behavior that route-native x86 diagnostics do not yet
  own.
- `backend_prepared_lookup_helper` remains because it aggregates many helper
  families and asserts both success and failure/fallback status.
- AArch64 prepared lookup threading remains because route fact delivery is not
  yet proven independently for every consumed fact family.
- x86 and RISC-V wrapper tests remain because wrapper behavior is still partly
  constructed from prepared lookup state.

## Future Implementation Candidates

Each future implementation idea should pick one row, name the exact prepared
surface it preserves, and prove replacement coverage without weakening the
prepared oracle.

| Candidate | Scope | Minimum proof shape |
| --- | --- | --- |
| Route-native printer oracle | Add route-native printer assertions for one route family or one coherent printer surface. | Positive, negative, ambiguous, mismatch, and fallback coverage for the selected facts; prepared printer expectations unchanged. |
| Route-native CLI dump stage | Add a distinct route-native dump flag or stage beside `--dump-prepared-bir`. | Routing, focus, ambiguity, mismatch, and fallback tests; prepared dump snippets unchanged. |
| x86 route-native summary/trace | Replace one compatibility-derived x86 route-debug row family with route-native authority rows. | Summary/trace tests that label authority source and preserve fallback summaries. |
| Route-native helper oracle | Build a helper-oracle family for one semantic lookup group. | Success, missing fact, invalid input, ambiguity, mismatch, and fallback status tests; prepared helper oracle unchanged. |
| AArch64 route fact threading | Prove one route fact family arrives through target-local route views without prepared lookup handles. | Context-construction or lowering tests naming route authority, prepared fallback, and target-policy separation. |
| Target-wrapper route input | Build wrapper input from an already-proven route-native view. | Wrapper behavior tests with prepared-vs-route mismatch and fallback cases; no target-specific diagnostic-only BIR adapter. |

## Reject Signals

Reject a future slice as route drift if it does any of the following:

- Weakens, deletes, renames, or marks unsupported an existing prepared
  diagnostic, route-debug, wrapper, oracle, c_testsuite, or baseline
  expectation to claim progress.
- Replaces semantic route coverage with output-only relabeling, section-name
  matching, or formatting-only expected text changes.
- Adds testcase-shaped diagnostic shortcuts that recognize one named expected
  output or one narrow fixture instead of exposing a route-native semantic
  diagnostic.
- Claims production lowering success, full-suite greenness, or selected
  consumer migration as prepared diagnostic/oracle readiness.
- Uses prepared lookup data as the source of truth for a route-native oracle
  without labeling it as compatibility fallback.
- Moves target policy, ABI policy, address formation, register/storage choice,
  branch spelling, or final emission records into route-owned diagnostics
  without a separate owner decision.
- Combines broad prepared API deletion, draft 155 work, or `PreparedBirModule`
  field demotion with this diagnostic/oracle planning line.

## Contraction Prerequisites

Before any prepared diagnostic or oracle surface contracts, the follow-up plan
must cite this checklist and state:

- The single diagnostic/oracle surface being changed.
- The route family or consumer group in scope.
- The prepared oracle or compatibility adapter that remains.
- The route-native semantic authority that replaces prepared authority.
- The positive, negative, ambiguous, mismatch, and fallback cases that prove
  equivalent coverage.
- The existing expectations that must remain unchanged.

If any item is missing, the correct status is retained prepared oracle, not
prepared contraction readiness.
