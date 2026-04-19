# Prealloc CFG Generalization And Authoritative Control-Flow Facts

Status: Active
Source Idea: ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md
Activated from: ideas/closed/64_shared_text_identity_and_semantic_name_table_refactor.md

## Purpose

Turn idea 62 into an execution runbook that replaces bootstrap-shaped
control-flow recovery with authoritative prepared CFG facts built on the typed
identity layer that idea 64 established.

## Goal

Publish a stable shared-prepare CFG/control-flow contract so downstream
consumers use prepared predecessor, successor, branch-condition, and join
ownership facts instead of re-deriving them from local CFG shape.

## Core Rule

Do not add testcase-shaped branch or join matchers. New work must improve
general CFG ownership and consumer contracts.

## Read First

- `ideas/open/62_prealloc_cfg_generalization_and_authoritative_control_flow.md`
- `ideas/open/63_complete_phi_legalization_and_parallel_copy_resolution.md`
- `ideas/closed/64_shared_text_identity_and_semantic_name_table_refactor.md`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- `src/backend/mir/x86/codegen/`

## Scope

- build authoritative prepared CFG facts for BIR functions
- make join ownership and continuation semantics derive from shared analysis
- move downstream consumers to the shared prepared control-flow contract
- keep symbolic identity on `FunctionNameId` and `BlockLabelId` surfaces

## Non-Goals

- completing phi destruction beyond the CFG facts needed for truthful prepared
  ownership
- generic scalar instruction selection
- unrelated x86 emitter rewrites
- new public CFG contracts keyed by raw strings

## Working Model

- prepare owns the authoritative per-function CFG view
- prepared data publishes predecessor, successor, branch-condition, and join
  ownership facts keyed by semantic ids
- consumers query that prepared contract instead of reconstructing branch and
  join meaning from bootstrap CFG shapes

## Execution Rules

- keep control-flow meaning in shared prepare, not in x86-local reconstruction
- prefer one-time CFG analysis plus typed prepared facts over repeated
  shape-sensitive helper queries
- update `todo.md`, not this file, for routine packet progress
- require `build -> narrow proof` for every accepted code slice
- broaden validation when a slice changes shared prepare plus multiple
  downstream consumer families

## Step 1: Inventory The Existing Prepared Control-Flow Contract

Goal: map the bootstrap control-flow ownership that exists today so the
replacement work stays focused on shared prepared facts instead of ad hoc
consumer rewrites.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- current x86 consumer entry points under `src/backend/mir/x86/codegen/`

Actions:

- identify the current prepared branch, continuation, and join facts that
  consumers read
- note the remaining places where consumers still recover meaning from CFG
  shape or local ambiguity heuristics
- confirm the typed `FunctionNameId` and `BlockLabelId` surfaces from idea 64
  are the identity boundary for new work

Completion check:

- the route has a concrete list of authoritative facts to preserve, bootstrap
  heuristics to eliminate, and consumer entry points that must migrate without
  widening scope

## Step 2: Build An Authoritative Shared Prepared CFG Model

Goal: make shared prepare own per-function CFG, branch-condition, and join
  facts as a stable consumer contract.

### Step 2.1: Publish Typed Per-Block CFG Facts

Goal: expose prepared predecessor, successor, and branch-condition facts on
semantic ids instead of leaving those edges implicit in local CFG shape.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- any extracted shared prepare helpers required to keep the analysis coherent

Actions:

- add or reshape prepared structures so per-block predecessor, successor, and
  branch-condition facts are published on semantic ids
- keep the prepared surface consumer-oriented rather than leaking bootstrap CFG
  assumptions
- preserve `FunctionNameId` and `BlockLabelId` as the public identity boundary

Completion check:

- prepared functions publish consumer-readable per-block CFG facts without
  requiring downstream code to rediscover basic edge ownership from raw labels

### Step 2.2: Make Join Ownership Authoritative

Goal: drive prepared join ownership from graph analysis instead of ambiguous
post-hoc recovery.

Primary targets:

- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/prealloc.hpp`
- any shared join-analysis helpers needed to keep ownership derivation coherent

Actions:

- derive join ownership from authoritative predecessor and successor analysis
- remove ambiguity-sensitive fallback paths that leave join transfers only
  partially owned
- keep join facts aligned with the same typed prepared CFG contract published in
  Step 2.1

Completion check:

- shared prepare can publish stable join ownership for ordinary branch flows
  without relying on local ambiguity heuristics

### Step 2.3: Finish Contract-Strict CFG Handoff Surfaces

Goal: close the remaining contract-defining paths that still fall back to raw
branch labels after shared prepare already owns the CFG edges they need.

Primary targets:

- shared prepared-control-flow handoff helpers in
  `src/backend/mir/x86/codegen/`
- countdown and guard-oriented consumers whose correctness still depends on raw
  single-successor labels

Actions:

- replace the remaining raw single-successor branch-label reads in strict
  countdown and guard handoff paths with the prepared control-flow contract
- keep mismatched `PreparedBranchCondition` and
  `PreparedControlFlowBlock` metadata as hard contract failures instead of
  silently reopening local fallback recovery
- keep this step limited to the consumer paths that define whether the shared
  CFG contract is already authoritative, not to the broader migration work
  reserved for Step 3

Completion check:

- the remaining contract-strict countdown and guard handoff paths no longer use
  raw branch-label recovery once authoritative prepared control-flow data exists

## Step 3: Migrate Consumers To The Authoritative Prepared Facts

Goal: move current consumers onto the shared prepared contract so short-circuit
and materialized-select paths stop depending on bespoke CFG interpretation.

Primary targets:

- consumer paths under `src/backend/mir/x86/codegen/`
- shared helpers that currently reconstruct branch or join meaning from CFG
  shape

Actions:

- replace local branch/join reconstruction with reads from the prepared
  control-flow contract
- keep consumer changes narrow to the paths needed to consume the new prepared
  facts
- reject testcase-driven local matcher growth when a shared prepared fact is
  the right fix

Completion check:

- the Step 3 substeps together leave the main consumers reading prepared
  predecessor, successor, branch-condition, and join facts directly enough
  that CFG shape recovery is no longer the authoritative path

### Step 3.1: Keep Branch-Target Handoff Consumers Contract-Strict

Goal: finish and preserve the branch-target consumer families whose correctness
depends on authoritative prepared branch-condition and prepared-target facts.

Primary targets:

- guard-chain and countdown-oriented paths under `src/backend/mir/x86/codegen/`
- short-circuit and compare-branch consumers that still depend on exact branch
  target ownership

Actions:

- keep branch-target handoff helpers consuming authoritative prepared metadata
  instead of raw label recovery
- add or maintain proof that prepared-target or branch-condition drift fails or
  redirects according to the shared contract rather than local fallback shape
- avoid reopening Step 2.3 fallback cleanup unless the contract itself is
  still incomplete

Completion check:

- branch-target-sensitive consumers prove the prepared contract remains
  authoritative across guard-chain, countdown, short-circuit, and
  compare-branch drift cases

### Step 3.2: Migrate Join-Carrying Consumers Without Topology Recovery

Goal: keep compare-join and joined-branch consumer families reading
authoritative prepared join and entry ownership instead of inferring meaning
from passthrough topology.

Primary targets:

- joined-branch consumers under `src/backend/mir/x86/codegen/`
- compare-join helpers and related join-carrying handoff paths

Actions:

- consume prepared join and entry-target facts directly where joined-branch or
  compare-join lowering currently invites local topology reconstruction
- prove that true-lane, false-lane, and entry-label drift still follow the
  authoritative prepared contract when the carrier topology changes
- keep joined-branch behavior aligned with prepared ownership rather than
  testcase-shaped lane matchers

Completion check:

- joined-branch and compare-join consumers no longer rely on local passthrough
  topology recovery as the authoritative source of control-flow meaning

### Step 3.3: Close Remaining Consumer Families And Shared Helper Gaps

Goal: finish the residual Step 3 consumer families that still lack explicit
prepared-contract migration or proof, especially materialized-select and other
passthrough-heavy helpers outside the already-covered branch and join lanes.

Primary targets:

- materialized-select and passthrough-oriented consumers under
  `src/backend/mir/x86/codegen/`
- shared helper surfaces that still permit consumer-local CFG interpretation

Actions:

- identify the remaining consumer families that are not already covered by the
  Step 3.1 and Step 3.2 proof lanes
- migrate those families to prepared predecessor, successor, branch-condition,
  and join facts without widening into phi-completion or unrelated x86 rewrites
- add focused proof that the remaining consumer helpers stay aligned with the
  authoritative prepared contract under label or topology drift

Completion check:

- no Step 3 consumer family still depends on bespoke CFG interpretation once
  authoritative prepared control-flow data exists

#### Step 3.3.1: Finish Immediate Materialized-Select Passthrough Surfaces

Goal: close the remaining immediate selected-value consumer proofs so both the
joined-branch routes and adjacent compare-join helper surfaces stay aligned
with authoritative prepared ownership under passthrough drift.

Primary targets:

- immediate selected-value and immediate selected-value-chain consumers in
  `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
- the corresponding passthrough-heavy helper surfaces under
  `src/backend/mir/x86/codegen/`

Actions:

- keep the immediate joined-branch route proofs authoritative for direct and
  `EdgeStoreSlot` lanes when true-lane or false-lane passthrough topology is
  introduced
- add the adjacent immediate compare-join helper drift proof that is still
  missing after the route-level slices already landed
- do not reopen broader global-backed or trailing-join families while this
  immediate packet group is still incomplete

Completion check:

- the immediate materialized-select family no longer has any residual route or
  compare-join passthrough-drift gap for the current prepared-control-flow
  contract

#### Step 3.3.2: Finish Global-Backed Materialized-Select Variants

Goal: close the remaining same-module global, pointer-backed, and fixed-offset
materialized-select consumers without letting those routes fall back to local
topology interpretation.

Primary targets:

- same-module global selected-value consumers
- pointer-backed and fixed-offset global selected-value variants
- any adjacent compare-join route or helper surfaces still missing explicit
  prepared-contract drift proof

Actions:

- keep direct and `EdgeStoreSlot` lanes reading authoritative prepared
  ownership across the remaining global-backed families
- add only the missing entry-carrier or passthrough-drift proof surfaces for
  each adjacent family instead of broad test sweeps
- keep packet boundaries aligned to one residual family at a time

Completion check:

- the remaining global-backed materialized-select variants no longer depend on
  bespoke route or helper reconstruction when prepared control-flow data exists

#### Step 3.3.3: Close Trailing-Join And Residual Shared Helper Proof Gaps

Goal: finish the last passthrough-heavy trailing-join and shared helper
surfaces so Step 3 closes with no orphan consumer family outside the earlier
branch-target and join-carrying lanes.

Primary targets:

- trailing-join arithmetic and bitwise consumer families
- any leftover shared helper surface that still lacks explicit prepared
  contract proof after Steps 3.3.1 and 3.3.2

Actions:

- keep trailing-join and late shared-helper proof focused on authoritative
  prepared ownership rather than topology-shaped recovery
- prove the remaining residual helper paths on the narrowest adjacent surface
  that is still uncovered
- stop Step 3.3 once no consumer family still needs bespoke CFG interpretation

Completion check:

- the Step 3.3.3 substeps together leave no residual trailing-join or late
  shared-helper proof gap that would justify another broad catch-all packet

##### Step 3.3.3.1: Preserve Trailing Bitwise Helper Render-Contract Coverage

Goal: keep the trailing bitwise helper lanes explicitly covered so the compare-
join render contract stays authoritative for `xor`, `and`, and `or` families
in addition to their route-level prepared-control-flow proof.

Primary targets:

- trailing bitwise joined-branch helper proof surfaces in
  `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`

Actions:

- keep trailing bitwise helper proof limited to explicit compare-join render-
  contract coverage on one adjacent operator family at a time
- preserve the already-landed route-level and helper-level prepared-control-
  flow contract checks without reopening unrelated trailing arithmetic or shift
  families

Completion check:

- the trailing `xor`, `and`, and `or` helper lanes all have explicit compare-
  join render-contract proof aligned with the authoritative prepared contract

##### Step 3.3.3.2: Finish Trailing Arithmetic And Shift Helper Coverage

Goal: close the remaining trailing arithmetic and shift helper proof lanes so
the compare-join render contract is explicit for the trailing `mul`, `shl`,
`lshr`, and `ashr` families, not just their route-level ownership checks.

Primary targets:

- trailing arithmetic and shift helper proof surfaces in
  `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp`
- any directly-adjacent prepared compare-join helper path needed to expose
  those render-contract checks

Actions:

- add one operator family at a time for `mul`, `shl`, `lshr`, and `ashr`
  without widening into broader materialized-select or countdown work
- keep each packet scoped to helper-level render-contract proof on top of the
  already-landed joined-route ownership checks

Completion check:

- the trailing `mul`, `shl`, `lshr`, and `ashr` families all have explicit
  compare-join render-contract proof aligned with authoritative prepared
  ownership

##### Step 3.3.3.3: Audit Final Residual Shared Helper Gaps

Goal: confirm Step 3.3 closes without any leftover late helper surface that
still depends on bespoke CFG interpretation after the explicit trailing helper
lanes are covered.

Primary targets:

- any remaining late shared-helper surface under
  `src/backend/mir/x86/codegen/`
- the narrowest proof surface in
  `tests/backend/backend_x86_handoff_boundary_joined_branch_test.cpp` needed
  to close a real leftover gap

Actions:

- verify whether any residual helper path still lacks explicit prepared-
  contract proof once the trailing arithmetic and shift lanes are covered
- if one remains, close it with the narrowest adjacent proof packet instead of
  reopening finished families

Completion check:

- Step 3.3 no longer has any residual late shared-helper gap after the trailing
  arithmetic, shift, and already-covered bitwise helper lanes are accounted
  for

## Step 4: Validate The CFG Ownership Route

Goal: prove the new shared prepare CFG route without bundling phi-completion or
unrelated backend rewrites.

Actions:

- require a fresh `cmake --build --preset default` for each accepted slice
- use a backend-focused proving subset for routine packets
- broaden validation when a slice changes shared prepare plus multiple
  consumer families or when the route reaches a closure-quality checkpoint

Completion check:

- accepted slices build cleanly and the proving logs show consumer-visible CFG
  ownership behavior is stable on the chosen validation scope
