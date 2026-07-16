# Idea 803 Step 1: Placement And Conflict Baseline

Status: normative placement baseline for idea 803; detailed contracts remain
prospective until Steps 2–7 update their named owners

Root authority: [BIR architecture and A1–F3 stage spine](../../../src/backend/bir/README.md)

## Scope and reading rule

This document assigns each idea 803 design area to one provisional owner chain.
It does not add a phase, change stage order, claim implementation, or select the
detailed policies reserved for Steps 2–7. A named first owner is where semantic
meaning first becomes normative. Later mutation owners may only realize that
meaning through the exact products named in the row; they do not acquire a
second semantic authority.

All products below are immutable and keyed by the exact graph revision, target
profile/rule version when applicable, and complete dependency fingerprints.
Graph mutation uses a private candidate, invalidates every observing product,
and publishes only after the named gate succeeds. Failure publishes no partial
capability or compatible-looking substitute.

## Affected owner inventory

| Owner | Existing authority relevant to 803 | Step 1 disposition |
| --- | --- | --- |
| [root BIR README](../../../src/backend/bir/README.md) | exact A1–F3 order, capability boundaries, retry rule, verifier profiles, and F1 handoff | links this single matrix; no stage row changes |
| [B3 CFG pass](../../../src/backend/bir/passes/cfg/README.md) and [CFG analysis](../../../src/backend/bir/analysis/cfg/README.md) | terminator-owned topology, exact edge occurrences, critical-edge normalization, derived CFG facts | first topology owner for `asm goto`; visibility remains B4-owned |
| [B4 SSA pass](../../../src/backend/bir/passes/ssa/README.md), [dominance](../../../src/backend/bir/analysis/dominance/README.md), and [publication/value flow](../../../src/backend/bir/analysis/publication/README.md) | SSA construction, phi inputs, value visibility, and whole-graph proof | first semantic owner for non-local-return SSA safety, `asm goto` snapshots, and bounded promotion |
| [B5 memory pass](../../../src/backend/bir/passes/memory/README.md) and [memory effects](../../../src/backend/bir/analysis/memory_effects/README.md) | target-independent memory identity, access/effect normalization, ordering, and effect facts | first memory owner for non-local return and semantic-object owner for over-alignment |
| [C6 address preparation](../../../src/backend/bir/preparation/address/README.md) | immutable target-keyed address/object/site requirements | first target-fact owner for over-aligned realization; never mutates BIR |
| [D4 target pseudo expansion](../../../src/backend/bir/passes/target/README.md) | complete target-specific one-to-many realization before allocation | sole pre-allocation mutation owner for explicit alignment/address operations and temporaries |
| [D5 out of SSA](../../../src/backend/bir/passes/out_of_ssa/README.md) | explicit edge copies and copy scratch before allocation, private copy resolution after stable E3 | consumer of edge-qualified SSA state; no new 803 semantic authority |
| [E1 liveness/interference](../../../src/backend/bir/analysis/liveness/README.md) | immutable allocation liveness, interference, pressure, fixed-home, clobber, copy, and scratch facts | first allocation-fact owner and consumer of exceptional boundaries/explicit D4 temporaries |
| [E2 allocator](../../../src/backend/bir/regalloc/README.md) | abstract assignment, coalescing, victim choice, eviction request, and correctness constraints | sole allocation-decision owner |
| [E3 spill/reload](../../../src/backend/bir/regalloc/spill_reload/README.md) | explicit spill/reload realization and the sole bounded `E3 -> E1` retry edge | realizes E2 decisions without choosing policy or frame locations |
| [E4 allocated publication](../../../src/backend/bir/allocated/README.md) | private frame draft/actions, final exact-current closure, frame plan, realizability, and atomic publication | sole frame taxonomy/packing/memory-home realization owner |
| [BIR verifier](../../../src/backend/bir/verify/README.md) | cumulative stage gates and exact-current Allocated/MIR-ready admission | subordinate stage-local proof authority, never a repair owner |
| [F1 construction](../../../src/backend/bir/machine_construction/README.md) and external [MIR boundary](../../../src/backend/mir/README.md) | strict one-record application of the verified E4 view, followed by machine-domain ownership | downstream apply-only consumer; no expansion, allocation, or frame repair |

These are the affected normative or consuming owners for the six rows. ABI,
target registry, diagnostics, and legacy/reference documents may supply factual
evidence in later steps, but they do not displace any owner listed here.

## Normative provisional placement matrix

| Design requirement | First semantic owner | Required upstream facts and exact keys | Graph mutation owner | Downstream consumer | Verifier / publication gate | Invalidation / recomputation | Failure behavior | Adjacency rationale |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Non-local return (`setjmp`, `longjmp`, `returns_twice`, and equivalent boundaries) | B4 owns SSA definition visibility and promotion legality; B5 owns retained memory identity, observability, ordering, and effects; [Step 2](step2_non_local_control_transfer_safety.md) closes the detailed contract | exact B3 CFG/dominance/value-flow for B4; exact B4 proof plus memory-effect/provenance facts for B5; later immutable exceptional-boundary and call-clobber facts must name their exact B4/B5-derived revision and target/call rule versions | B4 may construct SSA only according to its pre-mutation plan; B5 may canonicalize explicit memory/effect forms; E3 alone inserts an E2-selected spill/reload rewrite; E4 alone materializes required homes/frame actions | E1 consumes explicit uses/clobbers/reload boundaries; E2 constrains assignments; E3 realizes eviction; E4 realizes memory homes; F1 only applies the verified result | B4 whole-graph SSA gate, B5 memory/SSA gate, private E1/E2/E3 candidate gates, then E4 Allocated/MIR-ready gate | any B4/B5 graph/effect change invalidates value-flow, memory effects, liveness, assignments, spill state, frame, and realizability; E3 mutation returns only to fresh E1; E4 materialization triggers final projection/E1 and non-mutating E2/E3 validation | missing boundary facts, illegal promotion, stale register-only value, unsafe assignment, missing memory home, stale key, or unsupported realization fails closed at the detecting owner; no implicit allocator/F1 repair | B4/B5 are adjacent semantic owners before E allocation; immutable boundary facts avoid a second CFG, while E1–E4 perform only allocation/frame realization |
| `asm goto` instruction-point snapshots, including fallthrough, duplicate label edges, outputs, and clobbers | B3 owns exact successor-slot/edge-occurrence topology; B4 owns the instruction-point definition-stack snapshot and phi visibility semantics | exact B2 terminator intent and stable edge-occurrence identity for B3; exact B3 CFG, dominance, value-flow, instruction order, ordinary operand/result roles, and output/clobber facts for B4 | B3 alone normalizes/splits topology and rewrites edge references; B4 alone inserts/renames phi and SSA state against the resulting exact occurrences | B4 verifier and later D5 edge-copy planning consume edge-qualified state; E1 consumes explicit output/use/clobber state | B3 CFG publication gate followed by B4 whole-graph SSA gate; later cumulative gates preserve exact occurrence provenance | B3 topology edits invalidate CFG/dominance/SSA; B4 operand/order/phi edits invalidate value-flow and all later analyses; each is recomputed under the new exact revision | absent/ambiguous snapshot, wrong revision, output visible on a forbidden edge, missing duplicate occurrence, illegal phi input, or topology drift rejects the private candidate | B3 answers only “which edges exist”; adjacent B4 answers “which definitions each occurrence sees,” preserving one CFG authority and preventing block-end leakage |
| Bounded B4 promotion / phi-growth planning | B4, as the sole dynamic SSA construction and proof owner | exact B3 revision plus CFG/dominance/value-flow, stable object/value/edge order, static `SsaEligible` admission, and one versioned resource-policy key; Step 4 must choose the precise policy and metrics | B4 alone builds the selected complete promotion in one private candidate after the plan is accepted | B5 admits only a complete B4 checkpoint and either fully promoted selected values or an explicitly admitted memory form chosen by the Step 4 policy | B4 resource-plan validation plus whole-graph SSA publication; B5 admission confirms its input vocabulary without repairing B4 | input graph, policy version, eligibility, CFG, dominance, or value-flow change invalidates the plan; candidate failure discards all mutations and recomputation begins from immutable B3 | checked-arithmetic overflow, hard-bound breach, incomplete selected promotion, construction/resource failure, or SSA failure publishes nothing; mid-pass fallback is forbidden | planning sits immediately before the B4 mutation it bounds, and B5 remains the adjacent admission consumer; no resource pass or second SSA authority is needed |
| Versioned allocation cost facts and deterministic allocation/coalescing/eviction policy | E1 owns immutable cost/correctness facts; E2 owns all assignment, coalescing, victim, and eviction decisions | exact initial-D5 or E3-retry revision, current projected constraints, CFG/loop and optional profile inputs, call/clobber/fixed-home/rematerialization/copy/spill facts, finite target pools, and explicit policy/rule versions | E1/E2 do not mutate; E3 alone realizes an ordinary E2 eviction by explicit spill/reload insertion; D5 copy resolution and E4 frame work remain downstream | E2 consumes E1 facts; E3 consumes one exact E2 decision/progress witness; stable D5/E4 consume the final assignment and spill state | E1 coverage/fact validation, E2 allocation candidate validation, E3 private candidate gate, and final E4 non-mutating E2/E3 validation | any graph, pool, projection, constraint, profile, cost-policy, assignment, or spill change invalidates E1/E2; every E3 mutation returns exclusively to fresh E1; no retagging | uncovered identity, illegal fixed/group/scratch demand, nondeterministic or unbounded choice, no-progress/cycle/bound exhaustion, stale fact, or unrealizable rewrite fails closed | E1 derives facts after all allocatable D4/D5 identities exist; E2 chooses once from those facts; E3 realizes and uses the already accepted sole retry edge |
| Closed frame-object taxonomy and deterministic packing/sharing legality | E4, through one subordinate frame-layout contract to be completed in Step 6 | exact stable post-E3/D5-resolved candidate, assignments and spill objects, copy-resolution fingerprint, ABI/call/address requirements, dynamic-stack semantics, target layout/rules, object lifetimes/interference/escape, and all exact predecessor keys | E4 alone creates a private frame draft, assigns placements, and materializes bounded explicit frame actions before final exact-current closure | F1 consumes the exact `FrameRealizationPlan` and explicit actions; MIR receives only their one-record application | E4 private assigned-candidate checks plus final Allocated/MIR-ready gate prove taxonomy coverage, packing legality, action multiplicity, reachability, and realizability | any object, lifetime, interference, assignment, call/ABI, target, action, or graph change invalidates the draft and all final products; E4 recomputes final projection/E1 after action materialization and permits no repair loop | unclassified object, incompatible sharing, coincident-offset inference without proof, overlap/alignment/size overflow, unreachable placement, hidden action, stale key, or unsupported target realization rolls back all E4 staging | E4 already owns frame derivation and final publication immediately before apply-only F1; a subordinate taxonomy refines E4 rather than adding a phase or giving MIR layout authority |
| Fixed, dynamic, and runtime over-aligned object realization | B5 first preserves target-independent size, required alignment, extent, lifetime, observability, and static/dynamic character; C6 first derives target-specific requirements | exact B4/B5 semantic object and effects; exact Canonical plus C1–C5 target/layout/ABI/call facts, provenance, and versioned C6 address rules; later stages carry exact C6/D4/projection/E1/E2/E3/D5 lineage | B5 may canonicalize semantic memory/object form; D4 alone introduces every align/mask/round, address/large-offset operation and allocatable temporary; E3 realizes selected spills; E4 alone chooses region/base/padding/placement/restore and explicit frame actions | E1/E2 allocate every D4-exposed temporary; E4 consumes assignments and requirements; F1 applies one registered record per explicit allocated node/action | B5 memory/SSA gate, immutable C6 product validation, full D4 Pseudo reverification, E1/E2/E3 candidate checks, then E4 Allocated/MIR-ready realizability gate | semantic-object edits invalidate B5 and all target products; target/rule/layout changes invalidate C6 onward; D4 mutation invalidates graph analyses and projection; E3 retries E1; E4 mutation forces final exact-current closure | lost alignment/extent/lifetime, unavailable realignment/base/address form, unencodable offset without a D4 expansion, hidden temporary, allocation/frame infeasibility, unwind/non-local incompatibility, or missing one-record mapping fails at its owning gate | B5 preserves language semantics, C6 prepares without mutation, D4 exposes all target work before E1, E4 fixes layout, and F1 remains strictly apply-only |

## Gap and conflict ledger

No irreducible spine contradiction is proven by the current owners. The six
rows fit the existing B3–B5, C6, D4, E1–E4, and F1 adjacency. The following are
deliberately unresolved detailed contracts, not permission for an implementation
to invent behavior:

1. Resolved by [Step 2](step2_non_local_control_transfer_safety.md): B4/B5 now
   define exact non-local visibility and retained-memory facts, while E1-E4
   close clobber, residency, explicit rewrite, and frame-home realization
   without adding exceptional CFG authority.
2. Resolved by [Step 3](step3_asm_goto_instruction_point_snapshots.md): B3 owns
   explicit optional-fallthrough and label occurrence topology, while B4 owns
   one exact instruction-point snapshot and edge-qualified phi visibility.
3. B4 currently requires complete SSA but has no versioned promotion budget,
   checked metric, or selected fail-closed versus deterministic-partial policy.
   Step 4 owns that choice; this baseline does not preselect it.
4. E1/E2 state coverage, correctness, and determinism at category level but do
   not close the cost schema, policy version, profitability weights, stable tie
   breaks, or bounded eviction decision contract. Step 5 owns that closure.
5. E4 owns frame work but lacks a subordinate closed object taxonomy, sharing
   proof, deterministic packing order, reachability contract, and class-local
   failure bounds. Step 6 must add or converge that subordinate authority.
6. The B5 -> C6 -> D4 -> E1/E2 -> E4 -> F1 over-aligned route is architecturally
   placed but not continuous in the current local owner text. Step 7 must define
   fixed, VLA/runtime-aligned, large-offset, restore, unwind/non-local, and target
   rejection behavior without moving expansion past D4.

Potential wording tension, but not a phase conflict: the root stage table names
C6 as address-requirement authority while E4 currently refers generically to
“frame requirement identities.” Later documents must make C6's immutable
target requirement and E4's concrete placement distinct and explicitly keyed.
Likewise, E1's current “pressure facts” do not yet imply ownership of allocation
profitability decisions; those remain exclusively E2-owned.

## Baseline invariants for Steps 2–8

- Stage order remains exactly A1, A2, B1–B8, C1–C9, D1–D5, E1–E4, F1–F3.
- Terminators and exact edge occurrences remain the only persistent CFG
  authority; exceptional products and SSA snapshots cannot create a graph.
- B4 remains the sole dynamic SSA construction/proof owner.
- E1 facts are immutable; E2 alone chooses allocation policy outcomes; E3
  realizes one eviction and returns only to E1.
- E4 alone derives and verifies frame placement and materializes bounded frame
  actions before final publication.
- F1 remains one-to-one and apply-only. A missing expansion, temporary,
  assignment, spill, frame placement/action, or realizability fact is an
  upstream failure, never work for F1.
- All names for prospective facts, policies, or diagnostics in this audit are
  documentation vocabulary, not claims of landed types, `NodeKind` entries, or
  implementation.
