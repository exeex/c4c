# Accepted A1-F3 BIR Architecture Implementation

Status: Draft (parked; not active implementation authority)
Type: general BIR compiler architecture implementation
Architecture Checkpoint: `8a7404a265ab24e230dcf4d001d6d1033e8d9736`
Architecture Acceptance Commit: `edab15ee77b8a0695e43b890c3e4057b1a739f38`
Historical Transport Predecessor:
`ideas/closed/731_inline_asm_transport_and_regalloc_contract.md`
Historical Documentation Umbrella:
`ideas/open/732_bir_stage_document_convergence_umbrella.md`
Deferred Import Prerequisite:
`ideas/open/734_lir_to_new_bir_container_completeness.md`

## Intent

Implement the independently accepted A1-F3 BIR architecture as the general
backend pipeline: immutable Raw/Canonical publication, target preparation,
pseudo formation and legalization, shared allocation and spill retry,
allocated/MIR-ready publication, strict one-record machine mapping, and
emission boundaries.

This idea owns the shared compiler infrastructure and ordinary semantic route.
Closed idea 731 proved only the bounded LIR-to-new-BIR structured transport
seam. It carries no downstream feature, allocation, or late-assembly
obligations.

This proposal is parked under `ideas/draft/`. Idea 732 must first settle the
documentation ownership/order/seams, and idea 734 must later complete only the
target-independent LIR-to-new-BIR container/import boundary. Neither lifecycle
authorizes this draft's downstream implementation.

## Why This Idea Exists

The A1-F3 architecture is materially broader than any one inline-assembly
feature. It owns canonical passes, analyses, calls and ABI planning, target
layout, generic pseudo lowering, out-of-SSA, allocation, pressure spill/reload,
copy resolution, frame actions, MIR-ready publication, and multi-target machine
mapping. Keeping that work under idea 731 made completion of the general
compiler indistinguishable from completion of one feature consumer.

The architecture itself was converged and independently accepted before this
split. This idea changes lifecycle ownership, not the accepted technical route.

## Adopted History And WIP

The following landed commits were originally executed under the mis-scoped
idea-731 implementation runbook and are adopted as idea-733 progress:

- `fa43f618a` — exact BIR pipeline stage identity
- `e77652161` — deterministic BIR execution control
- `8a63a410f` — exact stage stamps on published BIR
- `e8320d7ef` — private BIR occurrence checkpoints

The proposed Step 1.1e packet belongs to this idea but is not present in the
current worktree. When executed, it is expected to add a private
`FunctionAttachmentTransaction`, mutation journal, rollback, revision/digest
refresh, and checkpoint tests in:

- `src/backend/bir/core/ir.hpp`
- `src/backend/bir/core/storage.hpp`
- `src/backend/bir/pipeline/checkpoint_internal.cpp`
- `src/backend/bir/pipeline/checkpoint_internal.hpp`
- `tests/backend/bir/backend_bir_checkpoint_test.cpp`

No implementation or proof for that packet is accepted by the lifecycle split.
The executor must create it as a fresh bounded packet and produce matching
proof before the supervisor can accept or commit it.

## Architecture Authority

- `src/backend/bir/README.md` owns the exact A1-F3 order.
- The accepted subordinate BIR Markdown contracts own local inputs, outputs,
  keys, invalidation, verifier gates, and failure behavior.
- `src/backend/mir/README.md` owns the strict apply-only F1 boundary.
- An implementation problem does not authorize a silent architecture change.
  Contract changes require renewed architecture review.

## In Scope

- stable IDs, revision stamps, immutable capability ownership, deterministic
  execution control, private transactions, cancellation, rollback, and exact
  product invalidation
- typed LIR import and Raw publication foundations required by A1-A2
- all mandatory B1-B8 canonical passes and their shared analyses/verifiers
- C1-C9 target binding, layout, preparation, constraint binding, and exact
  later-revision projection infrastructure
- D1-D5 generic pseudo lowering, shared call lowering, target legalization,
  Pseudo verification, out-of-SSA, copy reservation, and copy resolution
- E1-E4 liveness/interference, shared abstract allocation, explicit spill/reload
  retry, frame-action materialization, exact final products, and immutable
  `AllocatedBir`/`PreparedBir`/`MirReadyBirView` publication
- F1-F3 shared machine-boundary verification, registered one-record mapping,
  and integration with existing target emission facilities
- reviewed RV64, AArch64, x86-64, and i686 target data and mapping rules
- build integration, stage-local tests, cross-stage negative tests, broader
  regression checkpoints, and truthful implementation-status reconciliation

## Historical Transport Boundary

Closed idea 731 established the bounded structured LIR-to-new-BIR transport
carrier and ordinary value-identity seam. Idea 733 may preserve and consume
that landed interface, but does not inherit an obligation to complete a broader
idea-731 feature contract. General target preparation, constraint
infrastructure, allocation, MIR-ready publication, machine mapping, and
emission belong here when required by this idea's accepted A1-F3 architecture.
Future feature-specific constraint or late-assembly work requires a separate
explicit initiative.

## Out Of Scope

- reopening closed idea 731 by treating general infrastructure or downstream
  feature behavior as unfinished transport work
- restoring or compiling `src/backend/legacy/**`, old prealloc/MIR, removed
  `c4c-as`, or deleted `src/backend/bir/mir/**`
- target interpretation or allocation facts in Raw/Canonical BIR
- target-specific ordinary allocators or MIR/backend pressure repair
- broad unrelated object/linker/runtime redesign
- architecture changes hidden inside implementation packets
- testcase-shaped matching, named-case shortcuts, expectation downgrades, or
  classification-only changes claimed as compiler capability

## Acceptance Criteria

- Every accepted A1-F3 owner required by the active runbook has executable code
  or a truthfully bounded external owner, with exact build integration.
- All authoritative publications are immutable, exact-revision keyed,
  failure-atomic, and reject stale or mixed products.
- B1-B8 execute in the accepted order and publish target-independent,
  unallocated Canonical BIR.
- C1-C9 publish verified target/layout/preparation/constraint products without
  mutating Canonical storage or duplicating authority.
- D1-D5 expose all target-specific expansion, call transport, SSA destruction,
  and copy requirements before shared allocation.
- E1-E4 allocate every ordinary identity or fail closed, represent pressure via
  explicit BIR spill/reload, resolve copies, materialize finite frame actions,
  and publish one exact immutable MIR-ready revision.
- F1 consumes only that exact view, maps one BIR node to one registered machine
  record, and cannot allocate, expand, synthesize hidden frame work, or repair.
- Supported RV64, AArch64, x86-64, and i686 routes share the same BIR algorithms
  and differ only through validated target data and mapping rules.
- Step 1.1e and every later packet have fresh matching build and test proof
  before acceptance; milestone and final regression gates are green.
- Implementation-status documentation matches code, an independent final
  review finds no architecture/scope/overfit blocker, and remaining feature
  work is assigned to explicit consumer ideas rather than absorbed silently.

## Reviewer Reject Signals

- Reject any packet that changes the A1-F3 order, ownership, verifier interval,
  exact-current key route, or strict F1 boundary without renewed architecture
  review.
- Reject target facts, assignments, concrete registers, frame offsets, or
  target opcodes stored in Raw/Canonical BIR.
- Reject a target-specific ordinary allocator, MIR pressure spill/reload,
  hidden copy scheduling, hidden frame records, one-to-many F1 expansion, or a
  repair escape returned to BIR.
- Reject predecessor-key relabeling, copied products, stable-ID equality, or
  structural equality used as freshness proof.
- Reject partial publication after cancellation, verification failure, ID or
  revision exhaustion, allocation failure, or stale/mixed input.
- Reject accepting Step 1.1e from old logs or todo text without a fresh bounded
  implementation packet and proof over its exact worktree.
- Reject a named testcase shortcut, supported-to-unsupported downgrade,
  expectation rewrite, helper rename, or diagnostic reclassification claimed
  as semantic progress.
- Reject broad unrelated backend rewrites or claims that general
  infrastructure reopens or extends closed idea 731.
- Reject a new abstraction that retains mutable published state, ambiguous
  ownership, late allocation repair, or another exact old failure mode behind a
  new name.
