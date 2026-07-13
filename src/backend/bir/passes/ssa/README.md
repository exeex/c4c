# P04 SSA Canonicalization Pass Contract

Status: closed architecture contract. Implementation is deferred until the BIR
architecture is accepted.

`P04` is the `S05` function transformation. It consumes the exact committed
P03 output and publishes verified canonical SSA without materializing edge
copies or making allocation decisions.

## 1. Exact input and canonical form

The input is one immutable function view with exact epoch/module/function
revision and `PassProperty::CfgCanonical`. Exact-revision CFG and dominance
handles are required. The P03 postcondition, core def-use, and configured input
verifier must accept that same revision.

Canonical v1 uses phi semantics represented by the core's chosen phi or block-
argument form, never both within one function. Each incoming is keyed by the
exact `EdgeKey {source BlockId, successor-slot ordinal}` and carries one typed
`ValueId`. Its key multiset equals the destination's reachable incoming-edge
multiset exactly, including parallel edges. Incoming storage is ordered by
canonical edge order; vector position is not semantic identity.

## 2. Closed P04 authority

P04 owns deterministic SSA construction for admitted promotable definitions,
dominance-frontier phi/block-argument placement, renaming, incoming repair,
trivial-phi elimination, typed atomic RAUW, dead definition cleanup caused by
its rewrites, and final dominance/def-use verification. Each operation's
registry disposition says whether it defines ordinary SSA values, carries
memory effects, or is ineligible for promotion; unknown ownership fails.

For every rewrite P04 builds a closed plan containing inserted definitions and
stable IDs, exact incoming keys/values, all replaced uses, origin composition,
and deletions. Repair rules are complete:

- one definition dominates every ordinary use;
- phi/block-argument incoming values are checked on their particular incoming
  edge, not at the destination block entry;
- a new or remapped edge gets a value only through the SSA construction rule;
  there is no arbitrary duplicate-predecessor choice;
- parallel edges may carry equal or different values and remain separate;
- trivial elimination requires all live incoming occurrences to prove one
  replacement under poison/undef rules, followed by complete RAUW;
- split/merge remnants from P03 are accepted only when exact edge coverage and
  core def-use already agree; otherwise the occurrence fails.

P04 cannot mutate successor topology, choose physical homes or spill slots,
schedule edge transfers, interpret inline-assembly constraints, or create
target instructions. Legacy phi materialization is assigned to root stage
`D5`, after target legalization and before `E1` allocation liveness. D5 owns
parallel-copy planning and cycle-safe transfer realization on its private BIR
candidate; P04 only preserves the canonical SSA semantics that D5 consumes.

## 3. Transaction and exact output

`PassId::SsaCanonicalize` is a `Function` pass requiring `CfgCanonical`,
establishing `SsaCanonical`, and using `RepeatContract::Idempotent`. One
deterministically bounded construction/repair worklist reaches the registered
canonical form inside a single occurrence; the pipeline does not rerun P03 or
hide an outer fixed point.

The private transaction inventories all dispositions, reserves IDs, applies
the complete plan, derives its mutation summary, invalidates affected CFG,
dominance, publication/value-flow, liveness, and SSA analyses, recomputes the
post-edit checks on the candidate, and passes verifier-on-commit. The pipeline
publishes the whole function wave once. Success yields one immutable S05
revision with exact edge-key incoming coverage, complete def-use, dominance,
and no unresolved aliases. A true no-op keeps the revision.

Stale analyses, unsupported promotability, missing incoming values, malformed
def-use, dominance failure, nonconvergence, deterministic resource exhaustion,
cancellation, or verifier rejection rolls back the complete occurrence.
Failure publishes no phi, use repair, property, revision, analysis cache entry,
or stage token.
