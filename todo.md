# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 1.1
Current Step Title: Establish executable pipeline identity and transaction foundations

## Just Finished

- Completed Plan Step 1.1d: added an internal move-only
  `PipelineCheckpoint` that consumes verified Raw storage exactly once and
  exposes only a typed immutable view and exact `PipelineStageStamp`.
- Added a move-only, deep-owning `PrivateOccurrenceCandidate` fork of actual
  `ModuleData`. The candidate is immutable and discard-only: it has no editor,
  revision bump, promotion, Canonical publication, pass dispatch, or property
  claim surface.
- Forking uses the existing cancellation token and charges exactly
  `2 + function_count` deterministic work units: preflight, one unit per
  function in module order, and a post-clone safety checkpoint. Cancellation,
  deterministic budget exhaustion before or after cloning, and allocation
  failure have distinct typed outcomes and publish no candidate.
- The last-good checkpoint remains unchanged and reusable after successful
  forks, candidate destruction/discard, cancellation, and resource failures.
  An internal boolean ownership seam proves checkpoint and candidate never
  share the same `ModuleData` allocation without exposing raw addresses.
- Added dedicated checkpoint coverage for consume-once/move-only behavior,
  exact view/stamp preservation, deep ownership, repeated forks, discard,
  preflight cancellation, pre/post-clone budget failure, retry, and typed reuse
  failure for moved or discarded capabilities.

## Suggested Next

- Continue Plan Step 1.1 with a bounded exact mutation-journal,
  revision-bump, and atomic-promotion packet over the private occurrence
  candidate. Preserve last-good checkpoint ownership and keep pass dispatch,
  target facts, analysis products, and Canonical publication out of scope.

## Watchouts

- The checkpoint/candidate header is deliberately internal and is not exported
  through `bir.hpp`; only pipeline framework code should receive construction
  and fork authority.
- Fork budget accounting is deterministic, but a real allocator failure is
  reported as `AllocationFailure`, never as `ResourceLimit`. The allocation
  branch is structurally typed but not deterministically injected by this packet.
- The post-clone checkpoint deliberately destroys the unobservable private copy
  on failure. Builder construction remains revision zero; no mutation or
  revision increment exists yet.

## Proof

- `cmake --preset default`: configured and generated successfully.
- `cmake --build --preset default`: full default build succeeded.
- Exact supervisor-selected command:
  `ctest --test-dir build -j --output-on-failure -R '^backend_'`.
  Baseline `test_before.log`: 3/3 passed. Current `test_after.log`: 4/4 passed,
  including the new `backend_bir_checkpoint`; zero failures.
- Passed `git diff --check`.
- Owned files: `src/backend/bir/pipeline/checkpoint_internal.hpp`,
  `src/backend/bir/pipeline/checkpoint_internal.cpp`, minimal friend seams in
  `src/backend/bir/core/builder.hpp`, `src/backend/bir/core/ir.hpp`, and
  `src/backend/bir/core/view.hpp`,
  `tests/backend/bir/backend_bir_checkpoint_test.cpp`,
  `tests/backend/bir/CMakeLists.txt`, `todo.md`, and canonical `test_after.log`.
