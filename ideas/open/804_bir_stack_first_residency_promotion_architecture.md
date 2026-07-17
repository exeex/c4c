# BIR Stack-First Residency Promotion Architecture

Status: Open (approved; not active)
Type: Documentation-only BIR architecture rewrite
Related:
- `ideas/open/732_bir_stage_document_convergence_umbrella.md`
- `ideas/draft/733_accepted_bir_a1_f3_architecture_implementation.md`
- `ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md`
- `src/backend/bir/README.md`
- `src/backend/bir/LEGACY_COVERAGE.md`
- `ref/claudes-c-compiler/`

## Goal

Rewrite every affected BIR and adjacent Markdown contract so the first
correctness-complete Phase E route uses a stack-first, always-spilled abstract
baseline with bounded optional register-residency promotion instead of
register-first allocation followed by iterative eviction and spill/reload
repair.

The resulting architecture must guarantee that every ordinary
allocation-visible value already has a legal abstract `StackHome`, that
running out of allocatable registers merely stops further promotion, and that
the backend remains capable of reaching strict F1 machine construction without
a general `E3 -> E1` spill cascade. Register residency is an optimization over
a complete memory baseline, not a prerequisite for compilation correctness.

Preserve the A-F major phase spine if its existing ownership boundaries can
express this route. Revise the meanings and strict order of E1-E4 as needed,
and update every upstream or downstream contract whose current wording assumes
register-first allocation, eviction, hidden stack traffic, or allocation
repair.

## Why This Exists

The current BIR documents make ordinary register assignment a completeness
requirement. Register shortage produces an eviction request; E3 then inserts
new `Spill`/`Reload` nodes, invalidates liveness, and retries E1/E2. Although
that model can support an optimizing allocator, it makes the first working
backend responsible for spill-induced identities, renewed pressure, retry
progress, oscillation prevention, and cascade termination before the basic
pipeline can run end to end.

For initial bring-up, correctness and architectural observability matter more
than `-O2` allocation quality. A complete stack-resident route makes register
shortage an optimization limit rather than a correctness failure. It also
isolates the still-hard CFG, edge-copy, ABI, scratch, and frame obligations
instead of coupling all of them to iterative spilling.

This is not permission for target emitters to synthesize implicit loads,
stores, temporaries, or frame repair. The always-spilled baseline must remain
explicit and verifiable before F1.

## Authoritative Architecture Decision

The documentation converged by this idea must state all of the following as
normative baseline rules:

1. Every ordinary allocation-visible value begins with an abstract legal
   `StackHome` or another already fixed semantic/ABI home. Concrete frame
   offsets are not assigned at that point.
2. Register allocation is replaced, for the baseline route, by bounded
   register-residency promotion. A promoted value may receive a legal abstract
   register home; an unselected value remains legally stack-resident.
3. Exhausting an allocatable caller-saved or callee-saved pool is not an
   allocation failure and does not request general eviction, graph-changing
   spill insertion, or an `E3 -> E1` retry.
4. The baseline route is independently executable when no ordinary value is
   promoted. Promotion correctness is therefore proved by equivalence to the
   already legal stack route, subject to explicit memory, control-flow, ABI,
   and exceptional-control restrictions.
5. TargetProfile/C2 defines allocatable pools separately from reserved and
   fixed units, ABI roles, alias units, and guaranteed scratch classes/groups.
   Scratch sufficient to realize every admitted baseline pseudo family is a
   target capability obligation, not a hope deferred to F1.
6. D2 materializes calling-convention transport as explicit pseudo nodes using
   fixed abstract ABI roles, outgoing argument slots, result transport, and
   exact clobber sets. Those roles are pre-colored requirements. Residency
   promotion does not choose or rename ABI locations, but E1/E2 must model
   their occupancy, aliases, and clobber boundaries.
7. Residual stack traffic is explicit before F1. A memory-form pseudo may
   remain only when its target contract proves direct one-record realization;
   otherwise D/E must expose the required load, operation, store, address, and
   scratch work as explicit pseudo nodes.
8. F1 remains strict apply-only and one-record-per-admitted-pseudo. It cannot
   expand a stack operand into a hidden multi-instruction sequence, acquire an
   emergency register, repair an ABI transfer, allocate a frame slot, or
   revive the retired spill retry route.
9. Full optimizing allocation, live-range splitting, iterative eviction,
   advanced cross-block/cross-call residency, and post-F1 peepholes are future
   optimization scope. Their absence cannot block baseline acceptance.

## Required Stage Audit And Revised E-Phase Order

First audit the root stage table and every affected owner. Then publish one
normative revised table that preserves A1-D5 and F1-F3 except where wording
must change to support the new baseline. The documentation must settle the
exact E1-E4 meanings and order rather than leaving multiple possible routes.

The expected order to prove or minimally correct is:

| ID | Required baseline meaning | Required guarantee |
| --- | --- | --- |
| `E1` | Stack-home, liveness, occupancy, and promotion-fact analysis | Every ordinary identity has a legal abstract baseline home; exact-revision liveness, edge, call, alias, fixed-role, scratch, and profitability facts are complete. |
| `E2` | Bounded register-residency promotion | Select a deterministic legal promotion set from finite allocatable pools; unselected values retain `StackHome`; pool exhaustion succeeds by stopping promotion. |
| `E3` | Residual stack-traffic and transport materialization | Rewrite the private candidate so remaining stack reads/writes, call/edge transport, and scratch use are explicit or proven direct memory forms; perform no general victim selection or iterative spill retry. |
| `E4` | Copy closure, residual frame packing, final verification, and MIR-ready publication | Resolve remaining edge/call transports, pack only residual objects and ABI/frame obligations, materialize frame actions, prove direct realizability, and atomically publish the exact MIR-ready revision. |

If a different E1-E4 division is more consistent with existing owner files,
the documents may choose it only when they provide an exact responsibility and
mutation matrix showing that:

- no ordinary value lacks a legal baseline home;
- stack traffic is explicit before F1;
- promotion cannot require general eviction or graph-changing retry;
- D5 remains the sole semantic owner of out-of-SSA and parallel-copy rules;
- E4 remains the sole concrete frame/publication authority; and
- no new hidden phase or second allocation authority is introduced.

The root normal-success edge must no longer name `E3 -> E1` as a deliberate
retry. All old retry, eviction-request, repeated spill, progress-bound, stable
post-E3 allocation, and non-mutating E2/E3 replay wording must be removed,
rewritten as historical/future optimization material, or explicitly marked
retired from the baseline route. Renaming the same retry mechanism is not
acceptable.

## In Scope

### 1. Baseline homes and value taxonomy

Define a closed baseline home taxonomy and its owner. At minimum distinguish:

- ordinary non-addressable value `StackHome`;
- addressable, escaping, volatile, atomic, or otherwise observable memory
  object homes;
- fixed abstract ABI argument, result, return-address, stack-pointer,
  frame-pointer, and target-role homes;
- outgoing argument stack slots and incoming ABI homes;
- reserved scratch register classes/groups and explicit temporary stack
  objects;
- promoted abstract register homes; and
- values whose operation form is directly realizable from memory without a
  separate reload pseudo.

Specify identity, size/class/group, lifetime, alignment, revision keys,
ownership, legality, and invalidation rules. A `StackHome` is abstract and must
not embed a concrete frame offset before E4. Addressable semantic objects must
not be conflated with compiler-created ordinary value homes merely because
both eventually occupy stack storage.

Define when a result must be committed to its `StackHome`, when a load may be
forwarded, and how a fully promoted non-observable home becomes removable
without weakening volatile, atomic, escaped-address, `setjmp`/`longjmp`, or
inline-asm semantics.

### 2. TargetProfile and C2 baseline-realizability contract

Update the target-profile and BIR target-layout contracts to expose, with
stable target-keyed identities:

- allocatable caller-saved and callee-saved pools by category/class/group;
- reserved, fixed-role, stack/frame, link/return-address, and otherwise
  unavailable units;
- ABI argument/result roles and their concrete mapping domain;
- physical alias units and group/pair overlap rules;
- scratch-admissible classes/groups and guaranteed simultaneous capacities;
- operation-family scratch requirements, including address materialization,
  stack-to-stack copies, parallel-copy cycles, calls, and fixed-register
  operations; and
- target rejection when no baseline realization exists.

Do not express this as only `num_temp_regs`. The contract must distinguish
ordinary allocatable capacity from scratch capacity reserved to guarantee the
fallback route. It must also define whether a scratch is a fixed unit, an
abstract reserved role, or an explicit temporary object and how aliases are
checked.

### 3. C3/C4 and D2 calling-convention lowering

Keep ABI classification in C3/C4 and shared call mutation in D2. D2 must
replace a generic call with explicit, target-realizable pseudo-level boundary
transport including:

- argument moves/loads into fixed abstract ABI register roles;
- stores into abstract outgoing argument slots;
- hidden sret, byval, variadic, indirect-target, and aggregate transport;
- the call node and exact caller-saved, fixed, status, and target-specific
  clobber set;
- result extraction from fixed abstract ABI roles or result stack locations;
- immediate baseline storage of ordinary results back to `StackHome`; and
- exact preservation obligations that remain function-level E4 frame work.

The baseline call contract must permit all ordinary live values to survive in
their stack homes while scratch and caller-saved registers are treated as
clobbered. The first version may prohibit ordinary register residency across a
call. If so, state this as a verifier-enforced promotion admission rule, not an
implementation convention.

E1/E2 do not choose D2's ABI locations. They must nevertheless model them as
pre-colored occupancy, alias restrictions, fixed groups, and clobber
boundaries so promoted ordinary values cannot conflict with the call sequence.
Future callee-saved residency, around-call stores/reloads, argument/result
forwarding, and tail-call optimization remain outside baseline acceptance.

### 4. D4/D5 target and control-flow closure

D4 must expand every admitted pseudo whose baseline realization needs more
than one target instruction so all required temporaries, address calculations,
and scratch constraints are visible before E1. A target-proven one-record
memory form may remain, but the proof and mapping must be explicit.

D5 remains the out-of-SSA and parallel-copy semantic owner. The revised
documents must define:

- how exact edge occurrences carry stack-resident or promoted transport;
- critical-edge splitting and edge-local placement;
- stack-to-stack copy realization through guaranteed scratch or an explicit
  temporary stack object;
- simultaneous copy, overlap, and cycle correctness;
- how fixed ABI copies at calls interact with the same copy semantics without
  making D2 a second copy-resolution authority; and
- when copy resolution occurs relative to promotion and residual traffic
  materialization.

Stack-first allocation does not permit sequentializing a parallel bundle
incorrectly or placing path-specific traffic in an unsplit predecessor.

### 5. E1 baseline completeness and analysis

Define E1 inputs and exact-revision products for:

- baseline home completeness;
- ordinary and edge-qualified liveness;
- fixed/pre-colored occupancy;
- register aliases and simultaneous groups;
- call, inline-asm, and instruction clobbers;
- reserved scratch interference;
- loop/block/call-crossing facts used by bounded promotion; and
- correctness exclusions such as volatile, atomic, address-observable,
  exceptional-control, or unsupported cross-call values.

E1 must be meaningful even when the promotion set is empty. Missing homes,
insufficient scratch facts, incompatible fixed roles, stale products, or an
operation with no baseline realization fail closed. Ordinary allocatable-pool
pressure alone does not fail.

### 6. E2 bounded register-residency promotion

Specify a deterministic first-version promotion policy, suitable for a simple
linear scan, that:

- considers only explicitly eligible ordinary identities;
- uses finite allocatable pools without consuming reserved scratch/fixed units;
- respects aliases, groups, fixed roles, calls, inline asm, copy points, and
  target constraints;
- may conservatively restrict promotion to single-block ranges and/or ranges
  that do not cross calls, phis, exceptional boundaries, or unsupported
  control-flow joins;
- stops successfully when no register is available;
- leaves every unselected identity at its pre-existing `StackHome`;
- can replace a lower-benefit promotion only by reverting it to its existing
  legal baseline, without inserting a new spill scheme or changing the graph;
  and
- has stable ordering, benefit calculation, and tie breaks independent of
  pointer, hash, display, or incidental traversal order.

Promotion quality is not a baseline acceptance requirement. A zero-promotion
configuration must remain a supported proof mode. E2 must distinguish illegal
fixed/scratch conflicts, which fail closed, from ordinary lack of profitable
or available registers, which succeeds.

### 7. E3 residual stack-traffic materialization

Replace the old eviction-driven spill/reload owner contract with one explicit
baseline transformation. Define how E3:

- materializes reads from and commits to residual `StackHome` objects;
- removes or rewrites baseline traffic made unnecessary by legal E2
  promotion;
- materializes call-boundary and edge transport not already expressed by D2
  or D5 without stealing their semantic ownership;
- selects only target-guaranteed scratch roles or explicit temporary objects;
- preserves memory effects, exact edge placement, fixed roles, and constraints;
- rejects hidden multi-record work that would otherwise reach F1; and
- produces one new exact revision and projected constraint product for E4.

This stage must not select spill victims, create an eviction request, spill a
newly created reload temporary, or loop back to E1. If its rewrite reveals
that the target scratch contract was insufficient, publication fails and the
input remains unchanged.

### 8. E4 residual frame packing and publication

Define E4 as the only concrete frame and final allocation publication owner.
It must pack only objects that survive promotion and cleanup, plus semantic,
ABI, scratch, callee-save, dynamic, and frame-bookkeeping obligations.

The frame contract must cover:

- lifetime/interference-based reuse of ordinary residual `StackHome` objects;
- separation and legal coalescing among semantic locals, compiler value homes,
  outgoing/incoming ABI areas, explicit temporary objects, copy scratch,
  callee-save areas, and dynamic objects;
- size, alignment, over-alignment, variable extent, address escape, and fixed
  placement;
- deterministic packing and offset selection;
- large-offset/address-realizability constraints already exposed before E1;
- removal of fully promoted non-observable homes;
- materialization of frame actions; and
- final proof that every pseudo maps directly through F1 without allocation,
  scratch, ABI, or stack repair.

E4 publication remains transactional and exact-revision keyed. It must not
silently reintroduce an ordinary value home eliminated by promotion or hide a
late load/store sequence inside the frame plan.

### 9. Verifier and F1/MIR boundary

Update verifier profiles and the F1/MIR handoff to reject at least:

- an allocation-visible ordinary value with no legal baseline home;
- an unmodeled fixed ABI role, alias, clobber, or reserved scratch use;
- a promoted range crossing a forbidden call/control/effect boundary;
- ordinary register exhaustion reported as a compilation failure;
- residual implicit stack traffic requiring more than one machine record;
- a target pseudo whose scratch requirement exceeds C2's guarantee;
- unresolved `ParallelCopy`, hidden stack-to-stack transport, or illegal edge
  placement;
- concrete frame offsets introduced before E4;
- stale home, liveness, promotion, traffic, frame, or constraint products;
- F1-created temporaries, loads, stores, frame actions, ABI moves, or other
  repair; and
- implementation-status claims stronger than the landed code and proof.

F1 consumes only the final verified homes, explicit pseudo instructions,
frame realization, fixed-role mapping, and target mapping. External MIR and
target documentation must agree that machine construction and emission do not
own allocation recovery.

### 10. Legacy coverage and status truth

Rewrite `src/backend/bir/LEGACY_COVERAGE.md` and all affected implementation
status sections so legacy regalloc, preallocation, stack layout, spill/reload,
call transport, edge copies, and target scratch responsibilities map to the
new owners without implying that the old iterative allocator remains the
accepted baseline.

Documentation sketches, proposed node kinds, and future optimizing paths must
be labeled accurately. A complete Markdown design does not make an absent
implementation partial or complete.

## Required Documentation Outputs

At minimum audit and update, when affected:

- `src/backend/bir/README.md`, including the root stage table, normal-success
  edge, retry/failure text, invariants, and exact E1-E4 order;
- `src/backend/bir/target_layout/README.md` and the adjacent external
  TargetProfile contracts;
- `src/backend/bir/preparation/abi/README.md` and
  `src/backend/bir/preparation/calls/README.md`;
- `src/backend/bir/passes/call_lowering/README.md`;
- `src/backend/bir/passes/target/README.md`, pseudo schema, and target
  realization documents;
- `src/backend/bir/passes/out_of_ssa/README.md` and copy/scratch owners;
- `src/backend/bir/analysis/liveness/README.md`;
- `src/backend/bir/regalloc/README.md`, constraints, and
  `regalloc/spill_reload/README.md`;
- `src/backend/bir/allocated/README.md` and any subordinate frame-layout
  owner required to state residual packing precisely;
- `src/backend/bir/verify/README.md`;
- `src/backend/bir/machine_construction/README.md`, machine verification,
  external MIR, target, and emission boundaries where they repeat the old
  assumptions; and
- `src/backend/bir/LEGACY_COVERAGE.md`.

Use `rg` to find all references to `E3 -> E1`, eviction, retry, victim,
spill/reload insertion, stable post-E3 allocation, allocation completeness,
hidden memory operands, emergency scratch, and F1 repair. Do not limit the
rewrite to the likely owner list above.

The final documents must have one normative owner for each decision and linked
consumer summaries rather than independently drifting copies.

## Out Of Scope

- implementation code, tests, build changes, generated code, or runtime
  changes;
- activation, `plan.md`, `todo.md`, commits, or changes to draft 803;
- `-O2`-quality allocation or a performance claim relative to legacy or
  reference compilers;
- general iterative eviction/spilling, live-range splitting, graph coloring,
  aggressive rematerialization, or spill-cascade recovery;
- advanced cross-block, loop-carried, cross-call, or exceptional-control
  register residency beyond the conservative baseline admission rule;
- callee-saved residency optimization, around-call traffic optimization,
  argument/result forwarding, or tail-call optimization;
- post-F1 machine peepholes or assembly-text optimization;
- weakening CFG, critical-edge, phi, parallel-copy, ABI, inline-asm,
  `setjmp`/`longjmp`, volatile, atomic, or frame correctness; and
- routine generation of follow-up ideas. A separately scoped successor is
  allowed only if the audit proves a concrete contradiction that cannot be
  resolved while preserving the A-F major spine.

## Acceptance Criteria

This documentation-only idea is complete only when all of the following hold:

1. The root BIR table names one exact E1-E4 order implementing the
   always-spilled baseline, bounded residency promotion, explicit residual
   traffic, and final residual frame publication.
2. Every ordinary allocation-visible identity has a documented legal baseline
   home before promotion, and zero ordinary promotions is a supported route.
3. Ordinary allocatable-register exhaustion is documented and verified as
   successful non-promotion, not eviction or compilation failure.
4. All normative `E3 -> E1` spill-retry language is removed or explicitly
   retired from the accepted baseline. No consumer still requires stable
   iterative-spill products.
5. TargetProfile/C2 separates allocatable, caller/callee-saved, reserved,
   fixed, ABI, alias, and guaranteed scratch domains and proves baseline
   realization capacity.
6. D2 explicitly lowers calls into fixed abstract ABI-role and outgoing-stack
   pseudos with results and clobbers; E1/E2 treat them as pre-colored
   occupancy rather than allocation choices.
7. The first-version cross-call residency rule is explicit and verifier
   enforceable. Baseline arguments originate from legal homes and ordinary
   results return to legal homes.
8. D4/D5/E3 jointly expose all multi-record target, edge, call, and stack
   transport before F1 without duplicating semantic ownership.
9. Critical edges, parallel-copy cycles, stack-to-stack copies, and scratch
   shortages have exact legal and failure behavior.
10. E4 packs only residual objects with complete lifetime, interference,
    coalescing, ABI-area, scratch, alignment, and over-alignment rules.
11. F1 is still strict apply-only and one-record-per-admitted-pseudo; every
    forbidden hidden repair has a named verifier rejection.
12. The phase placement matrix names every producer, mutation owner, consumer,
    exact-revision product, invalidation rule, verifier, and failure mode.
13. `LEGACY_COVERAGE.md`, adjacent MIR/target boundaries, and all affected
    implementation-status statements agree with the new architecture.
14. No code, active lifecycle file, canonical regression log, existing draft
    803, or unrelated architecture is changed.
15. Structural proof shows no stale normative register-first retry contract
    remains in the affected documentation corpus.

## Proof

The later execution runbook must record at least:

```sh
git diff --check
git diff --name-only
rg -n "E3[[:space:]]*->[[:space:]]*E1|eviction request|spill/reload insertion|retry at E1|stable post-E3|spill victim" src/backend/bir src/backend/mir src/backend/target_profile
rg -n "StackHome|register-residency|promotion|pre-colored|ABI role|reserved scratch|one-record|apply-only" src/backend/bir src/backend/mir src/backend/target_profile
rg -n "Implementation-Status|Status:" src/backend/bir src/backend/mir src/backend/target_profile
```

The first search is an audit, not a blind zero-match requirement: historical
or explicitly future-optimization discussion may remain only when clearly
non-normative and non-baseline. Acceptance must include a path-by-path review
of every match.

Also require a manually recorded placement matrix covering at least:

- an all-stack straight-line unary operation;
- an all-stack binary operation needing two source values;
- a direct and indirect call with register and outgoing-stack arguments;
- a call result returned to `StackHome`;
- a caller-saved clobber with an ordinary value live across the call;
- a diamond phi and a critical-edge copy;
- a two-way and three-way parallel-copy cycle;
- a stack-to-stack copy;
- a large or over-aligned stack access;
- an inline-asm fixed/clobbered role; and
- zero-promotion and partial-promotion configurations.

For each case, the proof must name the explicit pseudo sequence, required
scratch/fixed roles, E1/E2 admission result, residual objects, E4 frame
obligations, and why F1 performs no hidden repair.

## Reviewer Reject Signals

Reject completion if any of the following is present:

- The documents merely rename register allocation to promotion while ordinary
  values still require registers for correctness or pool exhaustion still
  triggers victim selection, graph mutation, or retry.
- `E3 -> E1`, eviction, repeated spill insertion, or cascade termination
  remains a normative baseline success path under a new abstraction name.
- A target emitter, F1, MIR builder, assembler, or instruction printer is
  allowed to discover a `StackHome` and silently synthesize an unverified
  multi-instruction load/use/store or emergency-register sequence.
- D2 spells target register names as shared semantic identity, reclassifies
  ABI arguments, or lets E2 choose locations that C3/C4 already fixed.
- Conversely, E1/E2 ignore fixed ABI roles and clobbers on the theory that
  call lowering already handled them.
- The design says only that a target has some number of temporary registers
  without defining scratch classes/groups, simultaneous capacity, aliases,
  fixed conflicts, and baseline failure behavior.
- Stack-to-stack, call-argument, phi, or edge copies are sequentialized without
  parallel-copy/cycle proof, or path-specific copies are placed before an
  unsplit critical edge.
- Promotion deletes a store/load for an escaped, volatile, atomic,
  exceptional-control-visible, inline-asm-visible, or otherwise observable
  home without the required semantic proof.
- E4 allocates a permanent independent slot for every value without specifying
  lifetime reuse and removal of fully promoted non-observable homes, or merges
  incompatible object kinds solely because offsets happen not to overlap.
- Concrete offsets, target register spellings, or late address expansions
  appear before their named owners, or F1 repairs an unencodable frame access.
- Ordinary lack of a profitable register is reported as unsupported input,
  resource exhaustion, or allocation failure rather than legal fallback.
- The zero-promotion route is undocumented, unverifiable, or unable to lower a
  representative admitted operation family.
- Implementation-status labels claim the rewritten architecture exists in
  code when only Markdown changed.
- Tests or examples cover only one named testcase, one target, or one
  straight-line operation while the contract claims target-shared CFG/call
  capability.
- Expectations are downgraded, supported cases are reclassified as
  unsupported, or diagnostics are weakened to make the baseline appear
  complete without explicit user approval.
- Helper renames, heading changes, classification-only edits, or moving old
  retry text between files are claimed as architecture convergence.
- The change absorbs `-O2` allocation, post-F1 optimization, implementation,
  unrelated CFG redesign, or another broad rewrite into this documentation
  idea.
- The exact old failure mode survives behind `residency repair`, `fallback
  allocation`, `late materialization`, `emergency scratch`, or another new
  name.
