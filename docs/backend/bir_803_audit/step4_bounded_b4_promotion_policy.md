# Idea 803 Step 4: Bounded B4 Promotion Policy

Status: normative policy rationale; the owning operational contract is the
[B4 SSA pass](../../../src/backend/bir/passes/ssa/README.md)

## Decision

B4 selects **fail-closed complete promotion** under the versioned
`CompletePromotionPolicyV1` documentation contract. The complete eligible set
is planned against immutable exact-B3 facts before mutation. If any checked
metric overflows, any finite hard bound is exceeded, or construction cannot
realize the exact plan, B4 publishes nothing.

## Alternatives evaluated

| Policy | What can be reconciled now | Fatal vocabulary gap | Disposition |
| --- | --- | --- | --- |
| Fail-closed complete promotion | The existing B4 matrix already requires complete dynamic SSA and B5 already admits only a complete B4 checkpoint. One total eligible set, exact phi closure, and all-or-nothing publication fit those owners. | None. Non-promotable forms remain so because of semantic classification, never because of the budget. | Selected and normative. |
| Deterministic partial promotion | A versioned stable ranking and prefix selection could make the choice repeatable. | B4 has no closed retained-object memory output for deliberately unselected eligible objects; B5 has no total row that distinguishes such an object from half-promotion or resource fallback. Defining legal memory state, mixed uses, demotion, and admission would invent a new schema in this packet. | Rejected for V1 and deferred to an explicit future schema change. It is not an implementation option or fallback. |

## Deterministic resource contract

The policy key includes exact module/function revisions, CFG, dominance,
publication/value-flow and NodeKind-schema fingerprints, plus the policy
version. The version covers metric meanings, unsigned arithmetic width, finite
per-metric maxima, total plan-byte maximum, stable enumeration and tie rules,
and diagnostic priority.

The plan counts eligible objects, definitions, renamed uses,
dominance-frontier visits, phi sites/results, exact phi incoming occurrences,
typed rename/copy operations, and projected candidate nodes/operands, per
function and module. The exact inclusive V1 maxima and unsigned 64-bit checked
counter width are normative in the B4 owner. Every
addition/multiplication is checked before evaluation and every result is
compared with its hard maximum. Duplicate predecessor occurrences are distinct
phi and edge costs even when they share a destination. Enumeration and ties use
stable function/block/instruction/object/value/`EdgeKey` identity in
lexicographic order, never names, pointers, container order, or rendered text.

The complete phi closure and projected construction cost are accepted before a
private candidate exists. Construction then must match the plan exactly.
Failure, cancellation, newly required growth, allocation failure, or verifier
mismatch discards the candidate atomically. There is no smaller-set retry,
mid-pass budget fallback, successful half-promotion, or publication of the
resource plan independently of its exact green B4 checkpoint.

## B5 and verifier handoff

B5 admits only the exact completely promoted checkpoint and verifies the
promotion-plan key/accounting as inherited P04 proof. It neither repairs nor
demotes promotion. Ordinary explicit memory forms remain legal only when the
closed B4 matrix classified them as non-promotable for semantic reasons.

The B4/P04 gate recomputes the plan key and metrics, checks every hard bound,
and requires exact equality between planned and actual phis, results, incoming
edge occurrences, renamed uses, nodes, and operands. Any partial selection,
stale key, unchecked arithmetic, duplicate-edge collapse, post-mutation budget
decision, or unplanned growth rejects the complete candidate.

## Determinism example

Two duplicate edges from one predecessor to one merge block are two exact
`EdgeKey` occurrences. If one promoted value requires a phi there, the plan
charges one phi site, one result, and two incoming occurrences. Reordering hash
storage cannot change the count or order. With the same exact graph and policy
version, both the accepted plan and the first failing diagnostic are identical.
