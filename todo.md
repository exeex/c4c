Status: Active
Source Idea Path: ideas/open/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Draft The Common Fail-Closed Matrix

# Current Packet

## Just Finished

Step 2 - Draft The Common Fail-Closed Matrix completed as an analysis-only
matrix. These common rows are reusable across the calls, memory, edge
publication, source-producer, names, control-flow, and store-source families.

| Common row | Prepared surface | Required agreement or rejection condition | Reviewer reject signal |
| --- | --- | --- | --- |
| Public status/name preservation | Public helper/oracle names for prepared decoded home storage, formal publications, edge-copy/source facts, aggregate stack source authority, typed stack source publication, scalar publication, store-source publication, block-entry/current-block-entry publication, call-boundary classification, route statuses, target move intents, and wrapper kinds. | Any new route/BIR evidence must preserve the exact public spelling emitted by prepared status helpers and prepared printers, including `PreparedCallWrapperKind` names and target intent statuses. A change that only renames helpers, aliases enum names, or moves the spelling behind a classification wrapper is not proof. | Reject helper renames, status-string churn, renamed wrapper kinds, changed `--dump-prepared-bir` spelling, or claims that classification-only parity proves compatibility while public output names changed. |
| Fallback preservation | `PreparedFunctionLookups`, target `ConsumedPlans::prepared_lookups`, prepared call plans, prepared addressing/memory access readers, prepared publication readers, source producers, names, and value homes. | When route/BIR evidence is absent, stale, mismatched, incomplete, duplicate, or policy-insufficient, target lowering and prepared printer/debug output must continue using the prepared fallback path and preserve the previous output. Agreement may annotate or gate use, but disagreement must not transfer authority away from prepared facts. | Reject removal of prepared lookup reads, unconditional route-first lowering, fallback deletion, route/BIR-only output replacing prepared dump rows, or named-case shortcuts that only preserve one known fallback test. |
| Unsupported fail-closed behavior | Target intent status surfaces for x86 and RISC-V edge publications and any prepared reader that reports unsupported source, destination, publication, memory, call, or wrapper conditions. | Unsupported route/BIR/prepared combinations must fail closed into an explicit unsupported or missing status while preserving prepared fallback behavior where the previous contract did so. Unsupported must not be silently treated as available, and unsupported rows must not weaken expected contracts. | Reject expectation rewrites that downgrade supported behavior to unsupported, changes that turn unsupported into available without semantic proof, or classification-only changes that hide unsupported reasons behind a generic success path. |
| Expected-string stability | Prepared printer rows, route-debug rows, prepared dump CLI output, wrapper strings, grouped authority summaries, and target-policy-sensitive expected asm snippets. | Expected strings may change only when the prepared compatibility contract intentionally changes through source intent; route agreement rows must prove old strings remain stable for non-agreeing evidence. Tests must validate the observable string surface, not only internal enum classification. | Reject baseline churn, weaker substring-only assertions that drop compatibility text, helper-renamed output, or expectation rewrites used as proof instead of preserving the public surface. |
| Missing or invalid evidence | Missing shared lookups, missing publications, missing source/destination homes, missing names, invalid route indexes, absent route rows, malformed route references, and missing prepared evidence. | Missing or invalid route/BIR evidence must reject route authority and keep prepared fallback/output stable. Missing prepared evidence must produce the existing explicit missing status rather than inventing route-only authority. | Reject accepting nameless or invalid route facts, treating absence as agreement, fabricating raw-string names, or proving only by adding a classification that does not preserve prepared behavior. |
| Duplicate or conflict evidence | Duplicate route rows, duplicate/conflicting publications, conflicting names, conflicting source producers, conflicting call/memory facts, and ambiguous prepared-or-route ownership. | Duplicate or conflicting evidence must fail closed by rejecting route authority unless all identity, owner, key, value, and policy fields agree with the prepared surface. Ambiguity is not a reason to select an arbitrary route row. | Reject first-match or last-match shortcuts, testcase-shaped duplicate filters, conflict resolution by named case, or a pass condition that ignores one conflicting row. |
| Mismatch evidence | Wrong owner, wrong key, wrong successor/predecessor, wrong instruction, stale symbol/link-name ids, wrong value/home, wrong callsite/block index, and route/prepared name mismatches. | Route/BIR evidence may participate only when it matches the prepared identity and policy fields required by the family row. Any mismatch must preserve the prepared path or produce the existing explicit missing/invalid/mismatch status. | Reject broad equality shortcuts, raw-string name fallback authority, matching on display text alone, or changes that make a mismatched route fact drive target output. |
| Prepared-only preservation | Prepared-only surfaces including `--dump-prepared-bir`, prepared printer/debug rows, call wrapper output, prepared memory/GOT/address materialization behavior, and prepared target fallback asm. | If no agreeing route/BIR fact exists, prepared-only behavior remains authoritative and observable. Route/BIR rows may not be required for existing prepared output to appear. | Reject route/BIR-only replacement of prepared dumps, requiring route facts to print existing prepared rows, removing prepared-only target paths, or claiming compatibility through route coverage while prepared-only tests regress. |
| Route/BIR-only rejection | Route 1/4/5/6 availability/status rows, route index validation, route-debug annotations, and any BIR fact that overlaps a prepared call, memory, publication, source, name, or control-flow surface. | Route/BIR-only facts are rejected as proof unless they agree with the prepared identity, owner, key, status, and target policy required by the matching row. Route/BIR facts can gate diagnostics or validation; they cannot silently replace prepared authority. | Reject classification-only changes, route-only positive assertions, helper renames presented as proof, or named-case shortcuts that accept a specific route row without a general agreement check. |
| Policy-sensitive output stability | x86 call asm fallback, RISC-V `mv`/`lw`/`sw` edge moves, AArch64 prepared memory/GOT/address materialization, and target wrapper or publication behavior controlled by prepared policy. | Target output must stay stable when route/BIR facts are absent, stale, mismatched, incomplete, unsupported, or insufficient for the target policy. Route evidence can drive output only after proving policy-compatible agreement with prepared facts. | Reject output changes justified only by route availability, policy-insensitive lowering, expectation rewrites for target asm, or named-case materialization shortcuts that do not generalize across nearby policy cases. |

## Suggested Next

Proceed to Step 3 by drafting fact-family-specific matrix rows, starting with
calls and then extending the same pattern to memory, edge publications, source
producers, names, control flow, and store-source publications.

## Watchouts

The common matrix intentionally rejects helper renames, classification-only
changes, expectation rewrites, and named-case shortcuts as proof. Family rows
should reference these common rows instead of restating them, and should add
only the family-specific identity, owner, key, status, and policy agreement
fields needed for review.

## Proof

Analysis/documentation-only packet. No build, test, or `test_after.log` was
required by the delegated proof contract. Ran required proof command:
`git diff --check -- todo.md`.
