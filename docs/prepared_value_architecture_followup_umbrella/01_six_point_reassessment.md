# Six-Point Reassessment

Status: Step 3 complete

## Purpose

This document reassesses the six prepared-value architecture improvement
directions against the current closure and open-queue evidence. It classifies
what is already closed, what is owned by the existing Prepared MIR view
research line, what remains a candidate for later implementation or research,
and what should be deferred.

## Evidence Applied

- `ideas/open/595_prepared_value_architecture_followup_umbrella.md`
- `ideas/open/591_prepared_mir_view_contract_research.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`

## Status Categories

- Addressed or closed: the cited lifecycle records show a concrete authority,
  producer, or consumer route was implemented and closed.
- 591-owned: the problem belongs first in the Prepared MIR view contract
  research before implementation should start.
- New implementation candidate: the evidence names a concrete remaining
  capability gap with a likely first implementation owner.
- New research candidate: the evidence shows the ownership or semantic model
  is not yet concrete enough for implementation.
- Deferred: the evidence is intentionally too broad, adjacent, or not yet
  supported by a concrete first owner.

## Direction Summary

| Direction | Current status | First owning layer | Recommended disposition |
| --- | --- | --- | --- |
| Prepared publication model completeness | Partially addressed by closed freshness/publication chain; not globally complete | Shared-prealloc producer publication, then consumer authority and target consumption | Treat named 587-590 and 592-594 work as closed; map remaining families in Step 4 before generating narrow follow-ups |
| Move-bundle authority design | Substantially addressed for representative source-vs-destination ownership | Shared-prealloc consumer authority | Preserve closed ownership rule; consider only split follow-ups for select/alias or destination-fan-in families if Step 4 proves they remain uncovered |
| Value-home, preservation, and rematerialization priority | Addressed for MVP call and representative freshness uses; broader direct-home consumers remain deferred | Prepared/prealloc freshness authority, call-boundary freshness, and target consumption | Do not reopen generic priority work; create follow-up only for a concrete stale-home consumer not covered by 591 or closed authority paths |
| Pointer/address arithmetic and local-memory boundaries | Not closed by freshness chain; branch pointer stack-source freshness is only a narrow subset | Pointer/address semantic model | Candidate research split before implementation; avoid folding into branch freshness or MIR view migration without clear ownership |
| Call-boundary and post-call value publication | Partially addressed by 587; remaining call-shaped tails need evidence | Call-boundary freshness, shared-prealloc publication, and target consumers | Defer broad call ABI work; Step 4/5 may identify a narrow call-boundary follow-up if evidence shows a remaining stale publication gap |
| Diagnostic narrowing versus real capability closure | Policy risk remains; 591 already owns a major diagnostic/proof boundary question | Diagnostics/reviewer policy and target-independent MIR view contract | Keep diagnostic-only work out of capability claims; route Prepared MIR diagnostic boundaries through 591 unless Step 5 finds a separate fail-closed reviewer/verifier need |

### 1. Prepared Publication Model Completeness

Current evidence:

- Idea 587 introduced queryable prepared value freshness authority with
  explicit source kinds, use kinds, proof kinds, and precedence for
  representative call argument, move-bundle source, and
  producer-publication-operand uses.
- Idea 588 migrated dependency operand `LoadFromStackSlot` and inventoried
  remaining shared-prealloc gaps: branch stack-load authority, direct
  edge-publication moves, typed and aggregate stack-source publications, and
  select-carrier/select-alias authority.
- Idea 589 closed the direct edge-publication source ownership route by adding
  exact source freshness for current-block join parallel-copy sources.
- Idea 590 closed scalar branch condition stack-load freshness at branch
  terminator points and explicitly left pointer `Lhs`/`Rhs` blocked pending
  typed/aggregate producer publication.
- Idea 592 closed typed/aggregate branch stack-source publication for pointer
  `Lhs`.
- Ideas 593 and 594 closed RV64 pointer `Lhs` and `Rhs` branch stack-source
  consumption through selected shared freshness authority.

Current status:

Partially addressed and substantially narrowed. The original "publication
model incomplete" diagnosis is no longer true for the closed representative
families above. It is still not a global closure claim because the closure
notes leave aggregate-adjacent consumers, scalar-condition-register branch
shapes, string assembly emission, broad RV64/AArch64/x86 emission tails,
select/alias routes, destination fan-in, and predecessor-edge consumed
suppression outside the narrow queue.

First owning layer:

Shared-prealloc producer publication is the first owner when a missing
freshness fact prevents consumers from trusting a source. Shared-prealloc
consumer authority owns use-specific acceptance rules. Target RV64 consumption
owns only target consume paths after shared producer facts exist. The
target-independent MIR view contract owns later exposure of these facts to MIR
targets.

Recommended disposition:

Treat the named 587/588/589/590/592/593/594 chain as addressed or closed
evidence, not as active work to duplicate. Step 4 should map the remaining
uncovered families against idea 591 and current open inventory. Step 5 should
only create narrow follow-up ideas for concrete remaining producer or consumer
families with a clear first owner.

Reject or deferral notes:

Reject any follow-up that reopens "publication completeness" as a broad
umbrella implementation. Reject target-local freshness inference, expectation
downgrades, unsupported-marker edits, and diagnostic-only claims as
publication progress. Defer broad target tails until Step 4 proves they are
not already better handled by 591's view-contract research.

### 2. Move-Bundle Authority Design

Current evidence:

- Idea 587 made `MoveBundleSource` a freshness use/source vocabulary entry and
  rejected candidates whose only proof was the consumed move bundle itself.
- Idea 588 recorded prepared object move-bundle source consumption as already
  protected by idea 587.
- Idea 589 settled the direct edge-publication move ownership boundary:
  destination bundle legality, complete source homes, direct homes, alias
  evidence, and local move shape are not source freshness. The selected
  authority is the exact direct edge-publication row linked to the exact move
  resolution for the same source value and edge.
- Idea 589 left select-carrier alias, destination fan-in, predecessor-edge
  consumed suppression, and similar destination/alias legality routes as
  blocked on ownership design and split-worthy if continued.

Current status:

Substantially addressed for the representative move-bundle and direct
edge-publication source-vs-destination question. The remaining work is not a
generic move-bundle redesign; it is a set of adjacent authority questions
where alias, destination, or suppression facts may be mistaken for source
freshness.

First owning layer:

Shared-prealloc consumer authority owns the acceptance contract. Shared-prealloc
producer publication may become first owner only when a selected route lacks
the source freshness fact needed by that consumer.

Recommended disposition:

Classify the core move-bundle authority design as closed for the representative
families. Preserve the 589 ownership rule as a reviewer boundary. Step 5 may
consider a new narrow implementation or contract idea for select/alias or
destination-fan-in authority only if Step 4 shows no existing open coverage and
the route can name one first consumer.

Reject or deferral notes:

Reject accepting a source because a destination bundle, alias, or suppression
fact is legal. Reject overloading `MoveBundleSource` or
`DirectEdgePublicationSource` when a route needs a distinct use kind. Defer
mixed select/edge/destination work until it can be split by first owner.

### 3. Value-Home, Preservation, And Rematerialization Priority

Current evidence:

- Idea 587 established explicit precedence for direct homes, producer
  rematerialization, explicit publication, prior preservation, and move-bundle
  sources. Producer rematerialization and explicit publication outrank older
  preserved homes when valid for the same use.
- Idea 587 wired prepared call-plan publication and RV64 prepared call emission
  for stale-home-prone call arguments and retained fail-closed behavior when
  no authority can be selected.
- Idea 587 intentionally left broad RV64 object-emission value-home helpers,
  RV64 frame-slot/byval aggregate call paths, AArch64 call consumers, and x86
  Route6 integration outside the MVP.
- Ideas 588-594 continued the pattern of selected shared freshness authority
  before consumer acceptance instead of trusting structural homes.

Current status:

Addressed for the MVP priority rule and several representative use kinds, but
not globally closed for every direct value-home read or target-local consumer.
The remaining risk is concrete stale-home consumption in paths that still read
homes directly or rely on local structured validators.

First owning layer:

Prepared/prealloc freshness authority owns the priority rule. Call-boundary
freshness owns prior-preservation versus producer/rematerialization questions
at calls. Target RV64/AArch64/x86 consumption owns target use sites only after
shared authority is available.

Recommended disposition:

Do not create a broad "value-home priority" follow-up. Treat the priority rule
as closed where the 587 query is wired. Step 5 should create an implementation
candidate only if Step 4 identifies a concrete remaining stale-home consumer
with a first owner and proof surface. The Prepared MIR view research should
later classify which value-home or freshness facts are required codegen inputs
versus compatibility or diagnostic artifacts.

Reject or deferral notes:

Reject any route that makes an old preserved home win over a valid same-use
producer rematerialization or explicit publication. Reject target-local
ordering tweaks that do not publish or consume shared freshness authority.
Defer broad object-emission home replacement until a specific target consumer
or 591 view-contract migration owns it.

### 4. Pointer/Address Arithmetic And Local-Memory Boundaries

Current evidence:

- The 587 source idea identified older pointer/address cases as evidence
  anchors: pointer arithmetic materialization is insufficient unless the
  result is published to the prepared home later consumers trust, and computed
  global-address call arguments must materialize from semantic relocation
  rather than copied stale storage.
- Ideas 590, 592, 593, and 594 closed a narrower pointer branch-stack-source
  freshness path for pointer `Lhs` and `Rhs` branch operands. That path proves
  selected `BranchStackLoadSource` / `BranchStackSlot` freshness at exact
  branch terminator points.
- Those closures deliberately do not solve aggregate-adjacent branch sources,
  scalar-condition-register branch shapes, string assembly emission, broad
  target emission, or the general pointer/address semantic model.
- Idea 591's research covers MIR-facing view shape and feature contracts, but
  it does not by itself define the pointer/address semantic model.

Current status:

Not closed as a general architecture direction. The branch pointer
stack-source subset is addressed; pointer/address arithmetic and local-memory
boundary semantics remain a research candidate before implementation.

First owning layer:

Pointer/address semantic model is the first owning layer. Shared-prealloc
producer publication or target consumption should only become first owner
after the semantic model identifies which facts are authoritative for a
particular use. The Prepared MIR view contract may consume the result, but it
should not be used to hide unresolved pointer/address semantics behind a view
name.

Recommended disposition:

Classify as a new research candidate, not an immediate implementation route.
Step 5 should consider a separate pointer/address semantic-model research idea
unless Step 4 finds existing open coverage. Any later implementation should be
split by specific source publication or consumer path.

Reject or deferral notes:

Reject folding pointer/address semantics into the closed branch-stack queue.
Reject using stack-home completeness, local-memory layout, or target operand
shape as pointer freshness. Defer implementation until the first semantic
owner and acceptance criteria are concrete.

### 5. Call-Boundary And Post-Call Value Publication

Current evidence:

- Idea 587 wired call argument source selection through freshness authority,
  including producer rematerialization, explicit publication, direct homes, and
  unique complete prior preservation.
- Idea 587's closure inventory left RV64 frame-slot address/value and byval
  aggregate call-argument paths on structured validators, AArch64 call
  consumers on local selected-source/prior-preservation checks, and x86 Route6
  integration as separate.
- Idea 591 includes calls and related feature views in its Prepared MIR view
  research, asking how feature-specific contracts should be exposed without
  making them part of the core view.
- The 588-594 chain focuses on shared move/operand and branch stack-source
  freshness, not a full post-call publication sweep.

Current status:

Partially addressed. The highest-risk call-argument freshness priority was
covered by idea 587, but there is not enough current evidence in this Step 3
packet to claim post-call value publication is globally closed.

First owning layer:

Call-boundary freshness owns call-specific lifetime and prior-preservation
questions. Shared-prealloc producer publication owns missing post-call
publication facts. Target RV64/AArch64/x86 consumption owns target call
consumers only after the shared contract exists. The Prepared MIR view
contract owns how call feature facts are exposed to MIR.

Recommended disposition:

Do not reopen broad call ABI work. Treat 587 call-argument freshness as
addressed. Defer post-call publication follow-up generation until Step 4 maps
whether 591 already covers the interface question and whether a concrete
stale-home or missing-publication call path remains.

Reject or deferral notes:

Reject broad ABI, register identity, or call-lowering rewrites under this
umbrella. Reject local target fixes that bypass shared freshness. Defer if the
only evidence is a generic concern about post-call lifetime rather than a
specific producer or consumer gap.

### 6. Diagnostic Narrowing Versus Real Capability Closure

Current evidence:

- Idea 595 explicitly rejects capability claims based on diagnostics,
  expectation rewrites, unsupported-marker edits, allowlist filtering, or
  weaker runtime checks.
- Ideas 587-594 closed with implementation evidence, focused tests or prepared
  dumps, and regression logs rather than diagnostic-only narrowing.
- Idea 591 owns a dedicated diagnostic/proof boundary research question:
  classify prepared facts as codegen inputs, verifier facts, route proofs, or
  diagnostic/debug artifacts, and prevent diagnostic-only facts from becoming
  semantic authority.
- The repository AGENTS rules treat testcase-overfit and expectation
  downgrades as route drift, not progress.

Current status:

Still an active policy and contract risk, but not a standalone capability
implementation direction. The strongest existing owner is 591's Prepared MIR
view diagnostic/proof boundary research. A separate diagnostics/reviewer
follow-up may be useful only if it creates a fail-closed verifier or review
boundary that prevents diagnostic facts from driving codegen.

First owning layer:

Diagnostics/reviewer policy owns proof and reject boundaries. The
target-independent MIR view contract owns the Prepared MIR fact taxonomy that
separates required codegen inputs from diagnostic or proof artifacts.

Recommended disposition:

Classify as 591-owned for Prepared MIR view diagnostic boundaries and as an
intentional deferral for standalone diagnostic-only work. Step 5 should create
a separate follow-up only if the Step 4 mapping identifies a concrete
reviewer/verifier boundary not already covered by 591.

Reject or deferral notes:

Reject diagnostics, dumps, or route proofs as semantic codegen authority
unless a fail-closed verifier or explicit view contract makes the boundary
safe. Reject any follow-up whose main effect is narrowing diagnostic text,
rewriting expectations, or marking unsupported paths instead of closing the
underlying capability.
