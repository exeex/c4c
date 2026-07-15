# BIR Exceptional Control, Allocation, And Frame Design Completion

Status: Open (approved; not active)
Type: Documentation-only BIR design-completion umbrella
Related:
- `ideas/open/732_bir_stage_document_convergence_umbrella.md`
- `ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md`
- `src/backend/bir/README.md`
- `src/backend/bir/LEGACY_COVERAGE.md`
- `ref/claudes-c-compiler/`

## Goal

Complete the BIR and adjacent Markdown contracts for six known hard backend
design areas without changing the accepted A1-F3 phase spine:

1. `setjmp`/`longjmp`/`returns_twice` correctness across SSA, memory,
   liveness, allocation, spilling, frames, and verification;
2. `asm goto` instruction-point SSA snapshots and exact edge-qualified value
   visibility;
3. deterministic, bounded phi-growth and promotion planning;
4. explicit frame-object taxonomy and stack-slot packing/coalescing legality;
5. versioned, deterministic allocation cost, priority, coalescing, eviction,
   and spill policy; and
6. end-to-end over-aligned object planning and realization before allocation.

The result must be a coherent documentation contract that a later
implementation runbook can execute without rediscovering ownership, legality,
handoff, retry, verifier, or failure rules.

## Why This Exists

The current A-F architecture names the correct broad owners, but several of
the hardest legacy-backend responsibilities remain described only at the
category level. Comparison with the locked `ref/claudes-c-compiler` source
exposed concrete correctness and implementation policies that are either
unstated or insufficiently connected in the current BIR Markdown.

These omissions do not yet prove that a new major phase is necessary. They do
mean that an implementation agent could otherwise invent incompatible local
policies, hide late repair in F1, treat exceptional control flow as ordinary
block-end flow, or recreate the legacy coupling among CFG, SSA, allocation,
spilling, and frame layout.

This idea therefore converges the design directly in Markdown. It is one
design-completion source idea, not a triage-only generator. It must not create
follow-up ideas merely because several documents or phase owners are involved.

## Authoritative Architecture Boundary

- Preserve the exact A1-F3 stage order and the existing Raw, Canonical,
  Prepared, PseudoPreallocation, Allocated, and MirReadyMachine capability
  boundaries.
- Preserve B3 as CFG topology/normalization authority, B4 as dynamic SSA
  construction/proof authority, B5 as canonical memory/effect authority, E1
  as liveness/interference authority, E2 as allocation-choice authority, E3 as
  explicit spill/reload rewrite authority, E4 as frame/copy/final allocation
  publication authority, and F1 as strict apply-only machine construction.
- Preserve exact-revision products, private mutation followed by verification,
  atomic publication, stable identity/provenance rules, and fail-closed
  handling of stale or incomplete inputs.
- All one-to-many target realization required by an over-aligned access must be
  explicit before E1 allocation. F1 cannot synthesize hidden address
  calculations, temporaries, frame repair, or allocation work.
- A local contract may refine an owner or add a subordinate immutable product;
  it may not silently create a second semantic graph, CFG authority, allocation
  authority, or frame authority.

## Required A-F Placement And Conflict Audit

Before finalizing local contracts, audit every requirement in this idea against
the authoritative root stage table and all affected subordinate documents.
Produce a normative placement matrix that names, for each requirement:

- first semantic owner;
- required upstream facts and exact-revision keys;
- graph mutation owner, if any;
- downstream consumer;
- verifier and publication gate;
- invalidation/recomputation behavior;
- failure behavior; and
- why the placement preserves stage adjacency and does not require a new major
  phase.

The expected placement to prove or correct is:

| Design area | First owners | Required path |
| --- | --- | --- |
| non-local return semantics | B4/B5 | B4 -> B5 -> E1 -> E2/E3 -> E4/verifier |
| `asm goto` definition visibility | B3 facts, B4 semantics | CFG edge occurrence -> B4 rename/phi input -> verifier |
| bounded promotion planning | B4 | pre-mutation plan -> complete selected promotion or fail -> B5 admission |
| allocation costs and choices | E1/E2 | loop/call/rematerialization facts -> E1 cost facts -> E2 decision -> E3 rewrite/retry |
| frame-object packing | E4 | typed object requirements/lifetimes -> private frame draft -> verified E4 publication |
| over-aligned objects | B5/C6/D4/E4 | semantic object -> target address facts -> explicit pseudos -> allocation -> concrete frame -> apply-only F1 |

If the audit finds a real contradiction that cannot be resolved without
changing the major spine, stop that portion, record the exact incompatible
contracts and the smallest separately scoped architecture question, and do not
paper over it. Only such a proven conflict may justify a follow-up idea.

## In Scope

### 1. Non-local control transfer safety

Define an end-to-end contract for `setjmp`, `longjmp`, `returns_twice`, and
equivalent target/runtime facts:

- B4 promotion eligibility and SSA definition visibility around a
  `returns_twice` call;
- which locals or values must retain memory identity or memory observability,
  including the role of `volatile` and address escape;
- B5 effect, load/store, ordering, and preservation requirements for values
  visible after a non-local return;
- E1 liveness boundaries, exceptional successor or synthetic-boundary facts,
  call-clobber exposure, and reload requirements without inventing a second CFG;
- E2 assignment prohibitions or preservation requirements;
- E3 spill/reload placement and retry behavior;
- E4 object/frame obligations needed to keep non-local return state valid; and
- stage-local verifier failures for illegal promotion, stale values, missing
  memory homes, or unsafe assignments.

The documents must state whether correctness is represented through ordinary
edge occurrences, immutable exceptional-control products, semantic effect
facts, or a defined combination. It cannot remain an unspecified allocator
special case.

### 2. `asm goto` instruction-point SSA snapshots

Define the distinction between CFG topology and value visibility:

- B3/CFG analysis owns exact fallthrough and label edge occurrences;
- B4 records the definition-stack snapshot at the `asm goto` program point;
- label edges see precisely the state permitted at that instruction point;
- fallthrough visibility accounts explicitly for legal post-asm outputs;
- phi inputs remain keyed by exact edge occurrence, including duplicate edges;
- no later block-end definition may leak backward onto a label edge; and
- verifiers diagnose missing snapshots, wrong-revision snapshots, ambiguous
  output visibility, and illegal phi inputs.

The contract must cover zero, one, and multiple goto targets, fallthrough
presence/absence, output operands, clobbers, and interaction with critical-edge
normalization without using rendered labels or block names as identity.

### 3. Bounded phi-growth and promotion planning

Specify a deterministic resource model before B4 mutates the graph:

- define the measured cost or conservative bound, including phi count, copied
  incoming values, edge occurrences, or another explicitly versioned metric;
- define checked arithmetic, hard limits, diagnostics, and deterministic
  ordering/tie breaks;
- forbid mid-pass fallback, half-promoted objects, and success after an
  incomplete SSA construction;
- define how the chosen policy interacts with static `SsaEligible`, dynamic
  SSA proof, B4 output vocabulary, B5 admission, identity, and rollback; and
- require the same input graph and policy version to produce the same plan.

The planning decision must occur before publication-affecting mutation. Once a
promotion set is admitted, every selected object/value must receive complete
SSA construction or the private candidate must be discarded.

### 4. Frame-object taxonomy and stack-slot legality

Give E4 an explicit closed taxonomy and subordinate frame-layout contract. At
minimum account for:

- addressable and escaping locals;
- non-escaping fixed locals;
- explicit spill objects and reload homes;
- parallel-copy/cycle scratch objects;
- incoming homes, outgoing argument areas, by-value/sret/variadic ABI areas,
  and any fixed ABI-mandated object;
- callee-save storage and frame bookkeeping;
- fixed-size, variable-size, dynamic-lifetime, and runtime-aligned objects; and
- target-required emergency or address-materialization scratch only when it
  was explicitly introduced before allocation.

For every class, define size/alignment facts, lifetime source, interference,
address-escape implications, fixed placement, sharing/coalescing permissions,
forbidden sharing, dynamic/fixed ordering, offset reachability, deterministic
packing/tie breaks, failure bounds, and verifier evidence.

Explicitly decide whether and under what proof locals, spills, ABI areas, and
scratch objects may share storage. Object kinds with different semantic
lifetimes cannot be merged merely because their current offsets happen not to
overlap.

### 5. Allocation cost, priority, coalescing, and eviction policy

Define versioned E1 facts and E2 policy rather than leaving the central
allocator heuristic to implementation invention:

- loop nesting and optional profile-frequency inputs;
- use/def frequency, live-range shape/length, register-class pressure, and
  fixed-home constraints;
- call crossing and caller-saved/callee-saved tradeoffs;
- rematerialization eligibility and cost;
- copy/phi coalescing benefit and legality;
- spill load/store cost and callee-save amortization;
- eviction/progress rules and bounded retry compatibility with E3 -> E1; and
- stable deterministic ordering and tie breaks independent of pointer, hash,
  display, or incidental traversal order.

E1 supplies immutable facts; E2 owns assignment, coalescing, victim selection,
and eviction decisions; E3 realizes the selected rewrite and must not silently
choose a different victim or policy. Documents must distinguish correctness
constraints from tunable profitability weights and define how policy versions
participate in product keys or cache invalidation.

### 6. Over-aligned object end-to-end realization

Define one continuous contract:

- B5 preserves semantic size, required alignment, extent, lifetime, address
  observability, and dynamic/static character;
- C6 derives target-keyed natural alignment, runtime realignment need, legal
  address forms, offset ranges, base requirements, and dynamic-stack
  interaction without mutating Canonical BIR;
- D4 introduces every required one-to-many address calculation, large-offset
  materialization, align/mask/round pseudo, and allocatable temporary;
- E1/E2 allocate those explicit temporaries under ordinary rules;
- E4 chooses padding, aligned bases, object placement, frame-base actions,
  restore behavior, and relationships among fixed, dynamic, and over-aligned
  regions; and
- F1 applies the verified plan one record at a time without discovering or
  repairing alignment work.

Cover fixed and variable-size objects, over-aligned VLAs, stack realignment,
large/unencodable offsets, unwind/non-local-control interactions where
supported, and target rejection where a required realization is unavailable.

## Documentation Outputs

- Update `src/backend/bir/README.md` with the placement/conflict matrix and any
  clarified root invariants while preserving the A1-F3 order.
- Update every affected authoritative BIR owner under
  `src/backend/bir/**/*.md`, including CFG/SSA/memory, liveness, regalloc,
  spill/reload, allocated/frame, verifier, preparation/address, pseudo
  legalization, and machine-boundary documents as applicable.
- Update adjacent Markdown such as `src/backend/mir/README.md`, target/ABI
  documentation, or `src/backend/bir/LEGACY_COVERAGE.md` only when necessary
  to keep ownership and handoffs consistent.
- Add a subordinate Markdown owner for frame-layout policy or another missing
  local contract only when the ownership audit shows that an existing file
  cannot state it clearly. The new document remains subordinate to E4.
- Keep implementation-status statements truthful. Proposed types, algorithms,
  verifier APIs, and examples must be clearly labeled documentation sketches.
- Reconcile duplicated or conflicting statements so there is one normative
  owner and linked consumers, rather than copying the same rule into several
  independent authorities.

## Open Design Choice: Phi Budget And Partial Promotion

This draft intentionally does not pre-approve one policy. The documentation
work must evaluate and resolve exactly one of these contracts:

1. **Fail closed:** B4 computes the deterministic bound before mutation and
   rejects the compilation when the complete eligible promotion set exceeds
   the resource contract.
2. **Deterministic partial promotion:** B4 computes a complete, versioned
   promotion plan before mutation; selected objects receive complete SSA,
   while unselected memory-backed objects remain in a formally admitted
   non-promoted form accepted by B5.

Partial promotion is acceptable only if the documents define its static
classification, dynamic proof, memory-form legality, identity behavior,
deterministic selection, diagnostics, verifier rules, and atomic rollback. It
cannot be described as a fallback after mutation begins. If those requirements
cannot be reconciled with the accepted B4/B5 vocabulary, the accepted result
must be fail-closed or a separately scoped, explicitly proven architecture
conflict.

The selected policy and rationale must be recorded in the normative B4
contract and referenced by the root placement audit.

## Out Of Scope

- Production or test code, headers, build files, scripts, generated artifacts,
  binaries, or regression-log changes.
- Implementing any documented algorithm or claiming that documentation
  sketches are landed capability.
- Changing, activating, repairing, deactivating, or closing `plan.md`,
  `todo.md`, idea 732, idea 733, or another lifecycle source.
- Redesigning the shared BIR storage, stable-ID model, NodeKind/tag algebra, or
  accepted stage capabilities beyond the minimum clarification required by
  these six design areas.
- Adding a new major phase merely to obtain a convenient local owner.
- Machine-level post-F1 peephole optimization, machine rewrite publication,
  or renumbering F1-F3 to insert such a stage. That is a separate future
  architecture question.
- General optimization cataloguing, switch jump-table thresholds,
  compare/branch fusion, GEP folding, accumulator caching, or unrelated target
  tuning.
- Changing language semantics, weakening supported behavior, or classifying a
  supported construct as unsupported to avoid a design obligation.
- Generating follow-up ideas unless the required A-F conflict audit proves an
  irreducible separately scoped contradiction.

## Proof Approach

Because this idea is Markdown-only, acceptance proof is structural and
cross-document rather than runtime capability proof:

1. inventory the authoritative root and subordinate documents for B3-B5,
   C6, D4, E1-E4, F1, relevant analyses/verifiers, and adjacent MIR/target
   boundaries;
2. produce the requirement-to-owner placement/conflict matrix;
3. trace at least one normative end-to-end scenario for each design area,
   naming exact inputs, products, graph revisions, verifiers, invalidation,
   failure, and downstream consumer;
4. run repository Markdown/link/path consistency checks that already exist,
   plus focused `rg` audits for stale contradictory ownership statements;
5. compare the final documentation diff against the unchanged A1-F3 table and
   strict F1 boundary;
6. verify with `git diff --name-only` that the execution slice changed only
   permitted Markdown files; and
7. record unresolved implementation choices truthfully instead of marking a
   contract implemented.

Reference-backend behavior may motivate a requirement, but acceptance depends
on a self-consistent c4c contract. A reference implementation, legacy behavior,
or prose analogy alone is not proof that the c4c handoff is complete.

## Acceptance Criteria

- All six design areas have a single normative owner chain, explicit inputs
  and products, mutation and publication ownership, verifier rules,
  invalidation/recomputation behavior, diagnostics, and downstream acceptance.
- The A-F placement/conflict audit accounts for every requirement and proves
  that no new major phase is required, or stops and records the exact minimal
  separately scoped contradiction without pretending convergence.
- The root A1-F3 ordering is unchanged, and no subordinate document reorders
  stages or creates a competing CFG, graph, allocation, or frame authority.
- `setjmp`/`longjmp`/`returns_twice` behavior is specified across B4, B5,
  E1-E4, and verification sufficiently to prohibit stale register-only state
  after non-local return.
- `asm goto` label and fallthrough edges have explicit instruction-point
  definition visibility, exact edge-occurrence phi semantics, and verifier
  failures for illegal post-point definitions.
- B4 has one accepted deterministic phi-growth/promotion policy with checked
  resource bounds and no mid-pass fallback or half-promotion.
- E4 has a closed frame-object taxonomy and explicit lifetime,
  interference, placement, sharing, alignment, deterministic packing, and
  rejection rules for every admitted object class.
- E1 and E2 have a versioned division between immutable cost facts and
  deterministic assignment/coalescing/eviction policy; E3 realizes rather
  than re-decides the chosen rewrite.
- Over-aligned fixed and dynamic objects have a complete B5 -> C6 -> D4 ->
  E1/E2 -> E4 -> F1 route, with every allocatable temporary and one-to-many
  operation explicit before allocation.
- Every affected document agrees on exact-revision/product keys, failure
  atomicity, stable identity/provenance, retry boundaries, and strict apply-only
  F1 behavior.
- Implementation-status claims remain truthful and no documentation example is
  presented as executable proof.
- The final diff contains BIR or necessary adjacent Markdown only; no code,
  tests, lifecycle activation, canonical regression logs, or unrelated design
  changes are included.
- No follow-up idea is generated unless the conflict audit records concrete,
  irreducible evidence that one requirement cannot fit the accepted phase
  spine.

## Closure Note Requirements

Any later closure note must name the chosen phi-budget policy; list every
Markdown owner changed or added; summarize the six end-to-end owner chains;
state the result of the A-F conflict audit; identify any deliberately unresolved
implementation tuning; and record whether a separately scoped architecture
conflict was proven. It must not claim compiler capability was implemented.

## Reviewer Reject Signals

- Reject any change to A1-F3 ordering, a new major phase, or a weakened strict
  F1 boundary that lacks a concrete contradiction from the required audit.
- Reject `setjmp`/`longjmp` prose that stops at semantic flags and leaves SSA
  promotion, memory observability, liveness, register clobbering, spill/reload,
  or frame safety unspecified.
- Reject treatment of non-local return as an ordinary CFG edge when doing so
  creates a second topology authority or fails to model the actual program
  point and preserved state.
- Reject `asm goto` rules that use block-end state for every successor, allow
  post-asm outputs or later definitions to leak onto label edges, or identify
  edges by labels/rendered text rather than exact occurrences.
- Reject a phi budget applied after mutation begins, nondeterministic promotion
  selection, unchecked arithmetic, half-promoted objects, or a silent fallback
  whose memory form is not admitted and verified by B4/B5.
- Reject an E4 frame contract that only says "assign offsets" without a closed
  object taxonomy, lifetime/interference source, address-escape rule,
  sharing/coalescing legality, deterministic packing, and failure behavior.
- Reject storage sharing inferred from coincident offsets, display order, or a
  named testcase instead of proved non-interference and compatible object
  semantics.
- Reject allocator policy hidden in E3, target emitters, iteration order, hash
  order, or implementation folklore; reject cost facts without exact-revision
  and policy-version keys or deterministic tie breaks.
- Reject over-aligned-object repair in F1/emission, hidden temporaries after
  allocation, or a contract that omits dynamic objects, runtime realignment,
  large offsets, restore behavior, or target rejection.
- Reject copying the same rule into several Markdown files as independent
  authority rather than naming one owner and linked consumers.
- Reject a helper rename, heading shuffle, status relabel, reference-backend
  summary, or classification-only edit claimed as completion of an end-to-end
  design contract.
- Reject testcase-shaped shortcuts, named-case matchers, expectation rewrites,
  supported-to-unsupported downgrades, weaker verifier contracts, or diagnostic
  reclassification claimed as capability or design progress.
- Reject broad code, tests, build, unrelated backend, machine-peephole, or
  lifecycle changes inside this documentation-only idea.
- Reject a new abstraction that retains the exact old failure mode: implicit
  exceptional visibility, late hidden repair, ambiguous ownership,
  nondeterministic allocation, or unproved stack-slot aliasing behind a new
  name.
- Reject generation of routine follow-up ideas instead of directly converging
  the known Markdown gaps; only a documented irreducible phase conflict may
  leave this idea as a separate architecture question.
