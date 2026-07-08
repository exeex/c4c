# Follow-Up Idea Backlog

Status: Step 5 complete

## Purpose

This document records the Step 5 backlog decisions from the six-point
reassessment and current open-queue mapping. Each candidate family is either
mapped to existing idea 591, represented by a new narrow source idea, deferred
until a prerequisite supplies owner/proof detail, or declined as duplicate or
too broad.

## Decision Summary

| Candidate family | Owning layer | Disposition | Follow-up |
| --- | --- | --- | --- |
| Prepared publication remaining-family backlog | Shared-prealloc producer publication or consumer authority, split by selected family | Partially generated, otherwise deferred | Generated `ideas/open/598_select_carrier_alias_freshness_contract.md`; deferred aggregate-adjacent, scalar-condition-register, string assembly, broad target tails, destination fan-in, and predecessor-edge suppression until a first consumer and proof surface are isolated |
| Pointer/address value semantic model | Pointer/address semantic model | Generated new research idea | `ideas/open/597_pointer_address_semantic_model_research.md` |
| Call-boundary post-call publication and rematerialization | Call-boundary freshness, then shared-prealloc producer publication or target consumption | Deferred | No new idea until a concrete stale-home or missing-publication call path is named beyond idea 587's closed call-argument freshness and idea 591's call feature-view research |
| Diagnostic/proof artifact boundary | Target-independent MIR view contract and diagnostics/reviewer policy | Existing idea plus deferred standalone policy | Mapped to idea 591 for Prepared MIR diagnostic/proof taxonomy; no standalone diagnostics idea generated |
| Prepared MIR view integration amendments | Target-independent MIR view contract | Existing idea, no amendment now | Mapped to idea 591 unchanged |
| AArch64/x86 or other target consume-side migrations | Target consumer after shared authority exists | Deferred | No new target migration idea until shared authority and 591 view-contract prerequisites identify a concrete target consumer |

## Generated Ideas

### Pointer/Address Value Semantic Model

Disposition:

Generated `ideas/open/597_pointer_address_semantic_model_research.md`.

Evidence:

- `01_six_point_reassessment.md` classifies pointer/address arithmetic and
  local-memory boundaries as not globally closed.
- Ideas 590, 592, 593, 594, and closed 596 provide branch stack-source
  freshness evidence for narrow pointer branch operands only.
- Idea 591 can later consume MIR-facing pointer/address facts, but it does
  not own the semantic model that decides which facts are authoritative.

Owning layer:

Pointer/address semantic model.

Prerequisites:

- Closed freshness authority chain through ideas 587, 588, 589, and 590.
- Closed branch pointer stack-source evidence from ideas 592, 593, 594, and
  596.
- Idea 591 remains a later consumer, not a prerequisite for starting the
  semantic-model research.

In scope:

- Define what counts as semantic pointer/address identity, relocation meaning,
  local-memory boundary authority, stack/local address derivation, and
  published pointer value freshness.
- Classify existing pointer/address evidence into semantic authority,
  support/proof fact, target consume fact, or diagnostic-only artifact.
- Produce follow-up implementation splits only after the semantic owner is
  clear.

Out of scope:

- Reopening closed branch stack-source freshness for RV64 pointer `Lhs` or
  `Rhs`.
- Implementing target consumer migrations.
- Hiding unresolved pointer/address semantics inside the Prepared MIR view
  name.

Acceptance criteria:

- The research states an explicit semantic model and a fail-closed rule for
  each surveyed pointer/address family.
- It maps branch pointer stack-source freshness as a narrow closed subset,
  not as global pointer/address closure.
- It recommends one to three implementation follow-ups only when each has a
  first owner, prerequisites, and proof surface.

Reviewer reject signals:

- Reject testcase-shaped shortcuts, named-file routing, expectation
  downgrades, unsupported-marker edits, or target-local inference presented as
  semantic pointer/address closure.
- Reject broad mixed ownership that combines semantic modeling, shared
  producer publication, target RV64/AArch64/x86 consumption, and MIR view
  migration in one implementation route.
- Reject retaining the same stale-home, stack-home-only, local-memory-layout,
  or target operand-shape failure mode behind a new abstraction name.

### Select-Carrier Alias Freshness Contract

Disposition:

Generated `ideas/open/598_select_carrier_alias_freshness_contract.md`.

Evidence:

- Idea 588 left select-carrier and select-alias authority blocked on contract
  design, with existing alias closure evidence explicitly not sufficient as
  source freshness.
- Idea 589 closed direct edge-publication source freshness and identified
  select-carrier alias, destination fan-in, predecessor-edge consumed
  suppression, and similar destination/alias legality routes as split-worthy
  if continued.
- Step 4 found no open source idea covering a narrow select/alias acceptance
  contract.

Owning layer:

Shared-prealloc consumer authority.

Prerequisites:

- Closed idea 587 freshness authority MVP.
- Closed idea 588 shared-prealloc inventory.
- Closed idea 589 direct edge-publication source ownership rule, especially
  the rejection of destination-only and alias-only evidence as source
  freshness.

In scope:

- Audit the select-carrier alias acceptance surface and define whether a
  distinct freshness use/source kind is required.
- Decide which alias facts are support facts and which, if any, can be
  promoted to selected source freshness.
- Migrate at most one representative select-carrier alias consumer after the
  contract is explicit.

Out of scope:

- Destination fan-in, predecessor-edge consumed suppression, broad move-bundle
  redesign, and target backend consumer migration.
- Reusing `MoveBundleSource` or `DirectEdgePublicationSource` if doing so
  would blur the selected source authority.

Acceptance criteria:

- The idea identifies the audited select-carrier alias consumer set and states
  the exact ownership rule.
- Missing, ambiguous, stale, wrong-value, wrong-use, and alias-only authority
  fail closed.
- Focused proof shows the selected route uses explicit source freshness rather
  than destination, alias, or suppression legality alone.

Reviewer reject signals:

- Reject testcase-shaped matching, expectation downgrades, unsupported-marker
  edits, or allowlist filtering as proof.
- Reject broad mixed ownership that combines select alias, destination fan-in,
  predecessor suppression, producer publication, target migration, and MIR
  view design.
- Reject retaining the same alias-only or destination-only source freshness
  failure mode under a renamed helper or abstraction.

## Deferred Families

### Prepared Publication Remaining-Family Backlog

Disposition:

Partially represented by the generated select-carrier alias contract, but no
broad remaining-publication idea was generated.

Reason:

The remaining-family label covers unrelated first owners: aggregate-adjacent
branch stack-source producer publication, scalar-condition-register branch
shapes, string assembly emission, other target emission paths, select/alias
authority, destination fan-in, and predecessor-edge consumed suppression. A
single source idea for all of these would be a broad mixed-owner umbrella.

Deferred subfamilies:

- Aggregate-adjacent branch stack-source consumers: defer until a concrete
  source publication or consumer route identifies whether shared producer
  publication or shared consumer authority owns the first fix.
- Scalar-condition-register branch shapes: defer until evidence distinguishes
  branch source publication from target branch consumption.
- String assembly emission and other target emission paths: defer until a
  target route names the exact shared authority it must consume.
- Destination fan-in and predecessor-edge consumed suppression: defer because
  Step 3/4 evidence flags them as adjacent legality/suppression routes, not
  selected source freshness.

Reviewer reject signals for any later idea:

- Reject a broad "publication completeness" implementation.
- Reject accepting stack homes, destination facts, alias facts, or target
  operand shape as source freshness.
- Reject hiding multiple owners behind one follow-up title.

### Call-Boundary Post-Call Publication And Rematerialization

Disposition:

Deferred.

Reason:

Idea 587 closed representative call-argument freshness priority, including
producer rematerialization, explicit publication, direct homes, and unique
prior preservation. Step 3 identifies possible RV64 frame-slot/byval,
AArch64 call consumer, and x86 Route6 tails, but Step 4 does not isolate a
specific remaining stale-home or missing-publication call path with a first
owner and proof surface.

Prerequisites before opening:

- A concrete call consumer or producer gap beyond idea 587 must be named.
- The route must state whether call-boundary freshness, shared-prealloc
  producer publication, target consumption, or idea 591's feature-view design
  is the first owner.

Reviewer reject signals for any later idea:

- Reject broad call ABI rewrites or register identity changes.
- Reject target-local freshness bypasses.
- Reject retaining an old stale-home or prior-preservation failure mode under
  a new call-boundary helper name.

### Diagnostic/Proof Artifact Boundary

Disposition:

Mapped to idea 591 for Prepared MIR view diagnostic/proof taxonomy; standalone
diagnostics work deferred.

Reason:

Idea 591 already owns the question of classifying prepared facts as required
codegen inputs, verifier facts, route proofs, or diagnostic/debug artifacts at
the Prepared MIR view boundary. Step 4 did not identify a separate non-MIR
verifier or reviewer boundary that is concrete enough for a new source idea.

Prerequisites before opening standalone policy work:

- A concrete diagnostic/proof artifact must be shown to influence codegen
  authority outside the 591 MIR view boundary.
- The new route must create a fail-closed verifier or reviewer boundary, not
  just rename diagnostics.

Reviewer reject signals for any later idea:

- Reject diagnostics, dumps, route proofs, expectation rewrites,
  unsupported-marker edits, or allowlist changes as capability progress.
- Reject treating proof artifacts as semantic authority without an explicit
  required-fact promotion.

### Prepared MIR View Integration Amendments

Disposition:

Mapped to idea 591 unchanged.

Reason:

Step 4 found that idea 591 already covers Prepared MIR dependency inventory,
core and feature view design, diagnostic/proof boundaries, old/new BIR
equivalence, and MIR consumer migration planning. The umbrella handoff can
record the closed branch-stack authority evidence without changing 591.

Amendment trigger:

A plan-owner amendment is only justified later if 591 itself must explicitly
cite the closed branch stack-source authority or a newly generated
pointer/address semantic-model result as a required backend input.

Reviewer reject signals for any later amendment:

- Reject expanding 591 into producer-publication repair or target consumer
  migration.
- Reject exposing the full `PreparedBirModule` under a new view name.

### AArch64/X86 Or Other Target Consume-Side Migrations

Disposition:

Deferred.

Reason:

Ideas 593 and 594 closed only narrow RV64 fused pointer branch-stack `Lhs`
and `Rhs` consumption through selected shared authority. Step 4 evidence names
AArch64, x86, string assembly, and other target emission tails, but it does
not identify one target consumer with settled shared authority, proof command,
and migration boundary.

Prerequisites before opening:

- The shared producer or consumer authority for the selected target route must
  exist.
- Idea 591 or an equivalent contract must identify which MIR-facing facts the
  target should consume.

Reviewer reject signals for any later idea:

- Reject broad target sweeps.
- Reject target-local inference in place of selected shared authority.
- Reject proof limited to one named testcase while nearby same-feature target
  consumers remain unexamined.

## Declined Families

### Duplicate RV64 Pointer Branch Stack-Source Follow-Ups

Disposition:

Declined as duplicate closed work.

Reason:

Ideas 592, 593, 594, and closed 596 already cover the narrow RV64 fused
pointer conditional branch stack-slot `Lhs` and `Rhs` publication/consumption
queue through selected shared branch stack-source authority. Reopening that
same family would duplicate closed evidence rather than create new durable
source intent.

Reviewer reject signals:

- Reject rebranding the closed RV64 pointer branch-stack route as new
  pointer/address semantic work.
- Reject weakening or rewriting expectations around that closed queue as
  follow-up progress.
