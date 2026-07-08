# Current Open Queue Mapping

Status: Step 4 complete

## Purpose

This document maps the current open coverage and the closed branch-stack queue
residue so the umbrella does not generate duplicate follow-up ideas in Step 5.
It treats fresh closure records as authoritative over stale references to
ideas 592, 593, and 594 as open or queued work.

## Evidence Applied

- `ideas/open/591_prepared_mir_view_contract_research.md`
- `ideas/open/595_prepared_value_architecture_followup_umbrella.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `docs/prepared_value_architecture_followup_umbrella/01_six_point_reassessment.md`

## Current Open Inventory

Only two open source ideas remain relevant to this umbrella route:

| Idea | Current role | Coverage decision |
| --- | --- | --- |
| 591 | Prepared MIR view contract research | Existing open coverage for MIR-facing dependency inventory, core and feature view design, diagnostic/proof boundaries, old/new BIR equivalence, and MIR consumer migration planning |
| 595 | This umbrella triage route | Active classification and follow-up generator; not implementation coverage |

Ideas 592, 593, and 594 are no longer open queue entries. Their closed
lifecycle records are evidence for what the branch-stack freshness queue
already handled and what residue it deliberately left for later classification.

## Queue Entries

### Idea 591: Prepared MIR View Contract Research

Covered scope:

- Inventory current live MIR dependencies on `PreparedBirModule`, starting
  with x86, and separate live implementation dependencies from markdown-only
  legacy artifacts.
- Define a first `PreparedMirCoreView` shape and invariants that both old BIR
  and a future new BIR can produce.
- Split feature-specific view contracts for calls, variadic entry, i128/f128
  carriers, atomics, intrinsics, inline asm, and object data from the core
  view.
- Classify prepared facts as codegen inputs, verifier facts, route proofs, or
  diagnostic/debug artifacts so diagnostic-only facts do not become semantic
  authority.
- Define old/new BIR equivalence at the `PreparedMirView` boundary and propose
  a staged migration for MIR consumers.
- Recommend follow-up implementation ideas after the research answers which
  MIR-facing facts are required and which are optional or diagnostic.

Deliberate exclusions:

- No implementation changes to BIR, prealloc, MIR, tests, build files, target
  ABI classification, freshness authority behavior, branch or edge-publication
  semantics, runtime behavior, expectations, unsupported markers, or allowlists.
- No source repair for producer publication, target consumer migration,
  pointer/address semantic modeling, or call-boundary freshness.
- No guarantee that the entire current `PreparedBirModule` becomes the stable
  MIR ABI; the idea explicitly rejects exposing the full current module under a
  new name.
- No ownership of the closed 592/593/594 branch-stack implementation queue
  except as stable input evidence to classify when designing future MIR views.

Dependency or residue for future work:

- 591 can treat the migrated RV64 pointer fused-branch stack-source `Lhs` and
  `Rhs` routes as evidence that selected shared branch stack-source freshness,
  not stack homes or target-local structural facts, is the required authority
  for those backend inputs.
- 591 should not absorb unresolved pointer/address semantic-model work. It can
  consume that model later, but the semantic owner must first say what counts
  as authoritative pointer/address freshness or relocation meaning.
- 591 should not be used as the implementation route for remaining shared
  producer, shared consumer, or target-consumption families. It may classify
  their MIR-facing contract after those owners settle the capability.

Amendment recommendation:

Keep idea 591 unchanged for now. Its source intent already covers the
MIR-facing dependency, feature-view, diagnostic/proof, equivalence, and
migration questions needed by this umbrella. Step 5 should reference 591 as an
existing prerequisite or consumer for MIR-view work, not edit it. A later
plan-owner amendment is only warranted if the Step 5 backlog concludes that
591 must explicitly cite the closed 592/593/594/596 pointer branch-stack
authority as a required backend input; the current umbrella handoff can record
that without changing 591.

### Idea 592: Typed Aggregate Branch Stack Source Publication

Closed coverage:

- Audited branch stack-load producer paths and defined producer publication
  through branch stack-source freshness publication inputs.
- Required the matching source value, `BranchStackLoadSource` use,
  `BranchStackSlot` source, `BranchTerminatorOrdering` proof,
  `BranchStackSlot` rank, stack-slot home, and exact branch block plus
  terminator instruction point.
- Migrated pointer `Lhs` from inventory-only `policy=none` to selected
  `LoadFromStackSlot` authority gated by explicit producer-published branch
  stack-slot freshness.
- Preserved fail-closed behavior for missing, ambiguous, stale, wrong-value,
  wrong-use, future-point, and stack-home-only authority.

Deliberate exclusions and residue:

- Pointer `Rhs`, aggregate-adjacent consumers, select consumers,
  edge-publication consumers, RV64/AArch64/x86 target consumption or emission,
  and target-local branch freshness inference remained outside the 592 closure.
- The RV64 consume-side queue was intentionally handed to ideas 593 and 594
  rather than widened inside 592.
- 592 does not close the broader prepared publication backlog, select/alias
  authority, destination fan-in, pointer/address semantic model, or MIR view
  contract.

Coverage decision:

Do not generate a duplicate producer-publication idea for pointer `Lhs`
branch stack-source freshness. Treat 592 as closed producer-publication
evidence. Any future producer follow-up must name a different producer family
or one of the deliberately excluded residue families and must not reopen
592's completed `Lhs` branch terminator rule.

### Idea 593: RV64 Branch Stack Source Freshness Consumption

Closed coverage:

- Migrated the prepared RV64 object-emission fused pointer conditional branch
  path for stack-slot `Lhs`.
- Required selected shared
  `PreparedValueFreshnessUseKind::BranchStackLoadSource` /
  `PreparedValueFreshnessSourceKind::BranchStackSlot` authority for the same
  prepared source value, exact branch block, and terminator instruction index
  before RV64 emits the stack load into the `Lhs` scratch register and branch.
- Preserved the rule that layout, stack home, clobber, register, and operand
  shape facts are support facts, not freshness.
- Identified pointer `Rhs` as the next exact RV64 consume-side gap and sent it
  to idea 594.

Deliberate exclusions and residue:

- Pointer `Rhs` was not completed in 593 because the producer/collector still
  recorded it as inventory-only with `policy=none` / `status=missing_policy`.
- Aggregate-adjacent branch stack-source consumers, scalar-condition-register
  branch shapes, string assembly emission, AArch64, x86, and other target
  emission paths were intentionally deferred.
- Prepared MIR view design, old/new BIR equivalence, and MIR interface
  slimming remained the separate 591 line.

Coverage decision:

Do not generate a duplicate RV64 pointer `Lhs` consume-side idea. Treat 593 as
closed target-consumer evidence for the `Lhs` fused pointer branch route and
as the handoff source for the `Rhs` gap that 594 later consumed.

### Idea 594: RV64 Branch Stack Source Consumption Follow-Up From 593

Closed coverage:

- Completed the pointer `Rhs` branch stack-source consume-side gap left by
  593 after the producer-side blocker was cleared.
- Migrated the prepared RV64 object-emission fused pointer conditional branch
  route in `src/backend/mir/riscv/codegen/object_emission.cpp` so it queries
  selected branch stack-load source freshness for `Rhs`.
- Required selected `BranchStackLoadSource` / `BranchStackSlot` authority for
  the same prepared source value, `PreparedBranchStackLoadRole::Rhs`, exact
  branch block, exact terminator instruction index,
  `BranchTerminatorOrdering` proof, and `BranchStackSlot` rank.
- Confirmed that pointer `Lhs` and pointer `Rhs` fused pointer branch
  stack-slot consumers are now wired to selected shared authority.
- Stated that no additional numbered RV64 consume-side follow-up is required
  from this narrow pointer branch-stack queue.

Deliberate exclusions and residue:

- Aggregate-adjacent branch stack-source consumers, scalar-condition-register
  branch shapes, string assembly emission, and other target emission paths
  remain outside the 593/594 pointer consumer queue.
- AArch64 and x86 target migrations remain deferred.
- 594 does not settle the general pointer/address semantic model or the
  Prepared MIR view contract; it only supplies closed RV64 pointer branch
  consumer evidence that those later lines can consume.

Coverage decision:

Do not generate another numbered RV64 pointer `Rhs` consume-side idea. Treat
the 592/593/594 chain as closed for RV64 fused pointer conditional branch
stack-slot `Lhs` and `Rhs` consumption through selected shared authority.
Step 5 may still consider separate follow-ups for explicitly excluded
aggregate-adjacent, scalar-condition-register, string assembly, AArch64/x86,
or other target emission families if they have concrete owners and proof
surfaces.

## Existing Coverage Versus New Work

### Existing Open Coverage

- Prepared MIR view dependency inventory, core and optional feature views,
  diagnostic/proof boundaries, old/new BIR equivalence, and MIR consumer
  migration planning belong to idea 591.
- The umbrella classification and backlog generation belongs to idea 595.

### Closed Queue Residue

- The 592/593/594 branch-stack queue is closed for RV64 fused pointer
  conditional branch stack-slot `Lhs` and `Rhs` routes that consume selected
  shared branch stack-source freshness.
- The queue deliberately left aggregate-adjacent branch stack-source
  consumers, scalar-condition-register branch shapes, string assembly
  emission, other target emission paths, broad AArch64/x86 target migration,
  select consumers, edge-publication consumers, and target-local inference
  outside its scope.
- Those residue items are evidence for classification, not automatic new
  ideas. Step 5 must still decide which have enough owner, prerequisite, and
  proof detail for durable source intent.

### Genuinely New Follow-Up Candidates To Evaluate In Step 5

These candidates are not covered by 591 and are not closed by 592/593/594,
but this Step 4 packet does not generate source ideas for them:

| Candidate | First likely owner | Why it is not duplicate coverage |
| --- | --- | --- |
| Prepared publication remaining-family backlog | Shared-prealloc producer publication or consumer authority, depending on the selected family | 592 closed one branch stack-source producer path, while excluded aggregate-adjacent, select, edge, or target-emission families remain unclassified |
| Select/alias or destination fan-in authority | Shared-prealloc consumer authority | 589 and 592 reject using alias or destination facts as source freshness; no open idea currently owns a narrow acceptance contract for these adjacent routes |
| Pointer/address semantic model | Pointer/address semantic model | 592/593/594 close a branch pointer stack-source freshness subset, and 591 can consume MIR-facing facts later, but neither defines general pointer/address or relocation semantics |
| Call-boundary post-call publication gap | Call-boundary freshness or shared-prealloc producer publication | 587 handled representative call-argument freshness priority; no current open idea owns a concrete post-call stale-home or missing-publication path |
| Diagnostic/reviewer fail-closed boundary outside MIR view | Diagnostics/reviewer policy | 591 covers Prepared MIR diagnostic/proof taxonomy; a separate idea is only justified if Step 5 finds a non-MIR verifier or reviewer boundary needed to prevent diagnostic facts from becoming authority |
| AArch64/x86 or other target consume-side migrations | Target consumer after shared authority exists | 593/594 closed only narrow RV64 pointer fused-branch consumers; other target routes need separate owner and proof surface before source ideas are generated |

## Step 4 Recommendation

Idea 591 should remain unchanged. The umbrella should wait to amend 591 unless
Step 5 proves that the source idea itself must name the closed pointer
branch-stack authority as a required backend input. Existing open coverage is
therefore 591 plus this umbrella; closed branch-stack queue residue is
evidence for Step 5 classification; and genuinely new follow-up work should be
generated only when Step 5 can name a first owner, prerequisite set,
acceptance criteria, and reviewer reject signals.
