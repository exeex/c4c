# Common MIR Named Query Migration Runbook

Status: Active
Source Idea: ideas/open/706_common_mir_named_query_migration.md
Activated after completion of: ideas/closed/705_prepared_fact_boundary_from_bir_views.md

## Purpose

Remove route-shaped authority from the common MIR query boundary while
preserving target behavior and the ownership split established by named BIR
views and prepared facts.

## Goal

Replace direct Routes 1-8 and route-index queries in common MIR with named BIR
source-semantic results or prepared placement views, with explicit fail-closed
behavior and no route fallback.

## Core Rule

Common MIR may adapt or forward facts owned by BIR and prealloc, but it must
not rerun BIR analysis, reconstruct prepared authority, or absorb target policy.

## Read First

- `ideas/open/706_common_mir_named_query_migration.md`
- `docs/bir_mir_contract_abstraction/02_ownership_and_named_handoff_contracts.md`
- `docs/bir_mir_contract_abstraction/04_handoff_audit_and_closure_evidence.md`
- `src/backend/mir/query.hpp`
- `src/backend/mir/query.cpp`

## Current Scope

- Common query declarations and definitions in `src/backend/mir/query.*`.
- Named BIR source-semantic query consumption.
- Prepared MIR core/function/feature view consumption for placement facts.
- Common query contracts and directly affected x86, AArch64, and RV64 handoff
  tests through the existing target interfaces.

## Non-Goals

- Do not migrate target materializers owned by ideas 708-710.
- Do not implement the stack-destination authority gate owned by idea 707.
- Do not move target-specific policy into common MIR.
- Do not hide route records or indexes behind renamed generic wrappers.
- Do not weaken supported expectations or add route-discovery fallback.

## Working Model

- Named BIR views own source-semantic relationships and explicit availability.
- Prepared views own executable homes, moves, freshness, frame, publication,
  call-plan, and control decisions.
- Common MIR exposes narrow ownership-correct queries and fails closed when the
  required upstream fact is missing, incomplete, ambiguous, or mismatched.
- Targets retain current materialization behavior until their cleanup ideas.

## Execution Rules

- Migrate one ownership family at a time with positive and negative proof.
- Classify every route/index use before replacing it.
- Keep target compatibility changes bounded to signature/result adaptation.
- Run the supervisor-delegated build and focused proof for every code step.
- Run the common-query route-vocabulary guard and broader backend proof at the
  final integration step.

## Ordered Steps

### Step 1: Inventory common query authority and establish the guard

Goal: classify every route-shaped declaration, result, index, and fallback in
`mir/query.*` by semantic owner and establish the migration proof surface.

Actions:

- Inventory Routes 1-8 types, route indexes, analysis entry points, and route
  discovery fallbacks in common query declarations and definitions.
- Classify each query as BIR source semantics, prepared placement/authority,
  target compatibility, debug/proof, or out-of-scope materialization.
- Establish a focused guard requiring zero route headers, records, indexes, and
  hidden route-return wrappers in the final common query boundary.
- Identify directly affected registered contracts for all three targets.

Completion check:

- Every common query hit has an explicit owner and migration target, the guard
  detects route-shaped public/common contracts, and the first bounded producer
  family is selected without broadening into target policy.

### Step 2: Establish and consume source-semantic named results

Goal: replace common queries whose answers are owned by BIR with named BIR
results and explicit availability, creating the owning result before common MIR
adapts any semantic family that does not already have one.

#### Step 2.1: Establish the BIR-owned select/dependency result

Actions:

- Define a named BIR select-chain/dependency result with explicit status and
  stable root/dependency identity before changing the corresponding common MIR
  query.
- Preserve legacy Route 2 select-arm short-circuit behavior in the named
  result: an immediate, non-named, missing, or otherwise unresolved first arm
  is a stop/unavailable outcome and must not expose a dependency from the
  second arm. Keep that outcome distinct from a conclusively dependency-free,
  traversable first arm, which may continue to the second arm.
- Keep recursive interpretation of `SelectInst`, `CastInst`, `BinaryInst`, and
  `LoadGlobalInst` in the BIR-owned producer/view implementation; common MIR
  must not reconstruct that traversal from a same-block producer primitive.
- Represent at least complete direct-global dependency, complete no-dependency,
  and incomplete/ambiguous/mismatched outcomes in the named result.
- Add focused BIR-side proof for those outcomes and preserve the existing
  before-index and identity rules.
- Add operand-order compatibility proof for immediate-first/global-second and
  global-first/immediate-second select arms, alongside a conclusively
  dependency-free first arm that permits traversal to a later dependency.
- Do not broaden dependency discovery semantics in idea 706; any such semantic
  change requires separate ownership and target-policy planning.

Completion check:

- A BIR-owned named select/dependency producer and result expose the complete
  behavior-preserving Route 2 semantic answer with explicit status and stable
  identities. Focused proof covers direct-global positive, complete
  no-dependency, incomplete plus ambiguous/mismatched negatives, and both
  immediate/global operand orderings without allowing an immediate first arm
  to reveal the later dependency. No common MIR select-chain traversal is part
  of this completion claim.

#### Step 2.2: Adapt common MIR to named source-semantic results

Dependency: resume this step only after Step 2.1's behavior-preserving
select-arm semantics and operand-order proof are green.

Actions:

- Replace direct route-record and route-index inputs with the applicable named
  producer, memory, publication, call-boundary, or control query.
- For the select/dependency family, consume the Step 2.1 named result as a
  narrow status/identity adapter; do not recursively interpret BIR
  instructions or derive dependency completeness in common MIR.
- Preserve stable function, block, instruction, edge, value, and call identity.
- Reject missing, incomplete, ambiguous, unsupported, and mismatched applicable
  input without route discovery or agreement fallback.
- Add focused positive and negative common-query contract proof.

Completion check:

- Migrated source-semantic queries consume named BIR results only, preserve
  identity, fail closed explicitly, and expose no route-shaped result.
- Route-vocabulary guard counts may ratchet only after the corresponding named
  BIR result is consumed without semantic reconstruction in common MIR; Route
  2 reaches zero only after the select/dependency focused proof and broader
  backend proof are green.

#### Step 2.3: Enrich the BIR-owned memory-access result

Goal: establish a complete, bounded BIR-owned Route 3 memory-access payload
before common MIR attempts to consume that family.

Actions:

- Extend the named BIR memory-access result at its producer boundary so one
  complete result carries the instruction pointer, address space, volatility,
  alignment, slot/link identities, and the distinct local, global, and string
  names required by `BirMemoryAccessIdentity`.
- Preserve `instruction_index`, `base_name`, and the existing value identities
  where they remain part of the producer contract; do not make common MIR
  rediscover or synthesize the newly exposed fields.
- Bind every payload field to the same function, block, instruction, and value
  identity as the result status. Treat absent, incomplete, ambiguous,
  unsupported, or mismatched producer evidence as explicitly unavailable.
- Keep this substep limited to the named BIR producer/result and focused
  producer contracts. Do not change common MIR adapters, target placement
  policy, or materializers here.
- Add focused positive proof for applicable local, global, and string memory
  accesses and negative proof for missing or inconsistent identity payload.

Completion check:

- The named BIR memory-access result owns every field required to construct a
  `BirMemoryAccessIdentity`, preserves stable instruction/value/slot/link and
  local/global/string identity, and reports incomplete or mismatched evidence
  as unavailable. Focused producer proof is green, and neither common MIR nor
  target code reconstructs the payload.

#### Step 2.4: Adapt common MIR to the named memory-access result

Dependency: begin only after Step 2.3's complete identity payload and focused
producer proof are green.

Actions:

- Replace the bounded Route 3 common memory-access query with a narrow adapter
  over the Step 2.3 named result.
- Copy status and producer-owned identity into `BirMemoryAccessIdentity`
  without looking through BIR instructions, deriving names, or recovering
  address space, volatility, alignment, or slot/link identities elsewhere.
- Preserve the exact function, block, instruction pointer/index, value,
  address-space, volatility, alignment, slot/link, and applicable
  local/global/string identity exposed by the producer.
- Fail closed for missing, incomplete, ambiguous, unsupported, or mismatched
  results. Do not use route discovery, generic `base_name` inference, or
  agreement fallback.
- Add focused common-query positive and negative proof, then ratchet the Route
  3 vocabulary guard only for the fully migrated family.

Completion check:

- The common memory-access adapter consumes only the complete Step 2.3 named
  result, preserves every producer-owned identity field, and fails closed for
  every non-complete or mismatched status. Focused common proof and the
  supervisor-selected broader backend proof are green before Route 3 is
  claimed retired.

### Step 3: Migrate placement and executable-authority queries

Goal: make common placement queries consume prepared MIR views without
reconstructing prealloc decisions.

#### Step 3.1: Complete the prepared block-entry publication payload

Goal: make `PreparedCurrentBlockEntryPublication` the complete stable semantic
authority required by the common block-entry publication identity contract
before changing the common adapter.

Actions:

- Enrich `PreparedCurrentBlockEntryPublication` at the prealloc/prepared
  producer boundary with the stable successor and destination semantics that
  the common identity contract genuinely requires: successor label text and
  ID, destination value name text and ID, destination value ID and type, and
  the prepared publication bundle instruction index, alongside explicit
  result/proof status.
- Bind the complete payload to one prepared block-entry publication and its
  destination home. Reject missing, incomplete, ambiguous, unavailable, or
  mismatched names, labels, homes, publication bundles, types, and attribution
  rather than returning a partially authoritative result.
- Do not expose, retain, or reconstruct `bir::Block*`, `bir::Inst*`,
  `bir::PhiInst*`, or `bir::Value*` identity as prepared or common authority.
  Pointer-bearing proof inputs may remain only as bounded producer
  compatibility evidence while this substep establishes the stable payload;
  they must not become fields in the completed result.
- Keep this substep limited to the prepared producer/result and focused
  producer contracts. Do not change the common adapter, AArch64 caller,
  target materialization policy, or Route 5 families here.
- Add focused producer proof for one complete, uniquely bound publication and
  fail-closed missing, incomplete, ambiguous/unavailable, and mismatched
  successor/destination payloads.

Completion check:

- `PreparedCurrentBlockEntryPublication` carries every stable successor,
  destination, type, name, ID, status, and instruction-position field needed
  by the common semantic identity contract. Focused producer proof is green,
  and no raw BIR pointer is part of the prepared authority claim.

#### Step 3.2: Replace the Route 4 common adapter and adapt AArch64 compatibility

Dependency: begin only after Step 3.1's complete prepared payload and focused
producer proof are green.

Actions:

- Replace `find_bir_block_entry_publication_identity`'s Route 4 index build,
  validation, and raw-BIR reconstruction with a narrow adapter over the
  complete `PreparedCurrentBlockEntryPublication` result.
- Redefine the common result around producer-owned stable semantics: explicit
  status, successor label text/ID, destination value ID/name text/name ID/type,
  and publication instruction index. Retire obsolete request and result fields
  whose contract is `bir::Block*`, `bir::Inst*`, `bir::PhiInst*`, or
  `bir::Value*` identity; do not synthesize pointer identity from an index.
- Fail closed for every non-complete or mismatched prepared status without
  Route 4 discovery, agreement fallback, successor-block copying, instruction
  lookup, or PHI inspection in common MIR.
- Adapt only the signature/result compatibility in the AArch64 block-entry
  publication caller. Preserve its existing materialization decisions and do
  not move target policy into common MIR or expand into ideas 708-710.
- Add focused common-query and directly affected AArch64 positive/negative
  proof, then ratchet Route 4 vocabulary only for this fully migrated family.
  Do not include Route 5 edge/join migration in this packet.

Completion check:

- The common block-entry publication adapter consumes only the complete
  prepared result, exposes stable semantic identity without raw BIR pointers,
  and performs no Route 4 reconstruction. The AArch64 caller has only bounded
  interface compatibility changes, focused proof is green, and target
  materializer policy remains deferred to ideas 708-710.

#### Step 3.3: Continue remaining prepared-authority families

Actions:

- Replace other route-backed placement, publication, home, move, freshness,
  frame, call-plan, and control lookups with their existing complete prepared
  views in separately bounded packets.
- Require complete, uniquely identity-bound prepared authority and remove
  common-layer route agreement and discovery fallbacks.
- Add focused available and fail-closed placement proof for each family.
- Keep Route 5 edge/join migration separate from Steps 3.1 and 3.2 and do not
  combine it with the block-entry publication packet.

Completion check:

- Remaining placement queries forward prepared-owned authority, common MIR
  performs no route analysis or authority reconstruction, and negative inputs
  fail closed.

### Step 4: Adapt target-facing callers without migrating materializers

Goal: preserve x86, AArch64, and RV64 behavior through the new common interface
while leaving target policy for ideas 708-710.

Actions:

- Adapt affected target callers only to consume the ownership-correct common
  result and explicit status.
- Do not add target-side BIR analysis, prepared reconstruction, route fallback,
  or new materialization policy.
- Prove directly affected handoff behavior for each target with registered
  positive and negative contracts.

Completion check:

- All target callers compile and retain supported behavior through the common
  interface, with no target materializer migration or fallback authority.

### Step 5: Retire route vocabulary from the common query boundary

Goal: make `mir/query.*` independent of route headers, records, and indexes.

Actions:

- Remove obsolete route includes, parameters, return types, helpers, agreement
  branches, and analysis entry points from common query code.
- Ensure generic names do not conceal route-shaped payloads.
- Align common-query printers and contract fixtures with ownership-named facts
  without weakening expectations.

Completion check:

- The common-query guard reaches zero and public/common declarations expose
  only named BIR or prepared facts with explicit availability.

### Step 6: Audit ownership and run integration proof

Goal: prove idea 706 is complete without absorbing target cleanup or the
stack-destination authority gate.

Actions:

- Audit remaining `mir/query.*` route vocabulary and reject any executable or
  public-contract hit.
- Verify common MIR neither reruns BIR analysis nor reconstructs prepared or
  target authority.
- Run focused common-query and three-target handoff contracts plus the
  supervisor-selected broader backend regression check.
- Record target materializer work as ideas 708-710 scope and the positive
  stack-destination gate as idea 707 scope.

Completion check:

- Common MIR source-semantic queries use named BIR results, placement queries
  use prepared views and fail closed, route vocabulary is zero in
  `mir/query.*`, and broader proof is green without weakened tests.
