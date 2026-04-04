# Target Profile and Execution-Domain Foundation

Status: Active
Source Idea: ideas/open/40_target_profile_and_execution_domain_foundation.md
Supersedes: ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md (temporarily parked)

## Purpose

Establish one authoritative target/ABI profile and declaration-level
execution-domain model before more backend migration work hardens around the
current raw-triple, single-target assumptions.

## Goal

Introduce a shared `TargetProfile`, preserve `ExecutionDomain` in HIR, and make
`HIR -> LIR` ready for future multi-target splitting while keeping current
behavior host-only.

## Read First

- [ideas/open/40_target_profile_and_execution_domain_foundation.md](/workspaces/c4c/ideas/open/40_target_profile_and_execution_domain_foundation.md)
- [src/apps/c4cll.cpp](/workspaces/c4c/src/apps/c4cll.cpp)
- [src/frontend/sema/sema.cpp](/workspaces/c4c/src/frontend/sema/sema.cpp)
- [src/frontend/hir/hir.cpp](/workspaces/c4c/src/frontend/hir/hir.cpp)
- [src/codegen/lir/hir_to_lir.cpp](/workspaces/c4c/src/codegen/lir/hir_to_lir.cpp)
- [src/codegen/llvm/llvm_codegen.cpp](/workspaces/c4c/src/codegen/llvm/llvm_codegen.cpp)
- [src/codegen/shared/llvm_helpers.hpp](/workspaces/c4c/src/codegen/shared/llvm_helpers.hpp)

## Working Model

Execution should move in small, test-backed slices:

1. inventory current target/data-layout ownership
2. introduce canonical `TargetProfile`
3. thread it through frontend/codegen boundaries
4. introduce declaration-level `ExecutionDomain`
5. prepare `HIR -> LIR` for future split lowering
6. realign tests around centralized target/layout fixtures

Architecture rules for this runbook:

- `SourceProfile` remains a language/front-end mode, not a target/ABI profile
- `TargetProfile` owns target triple, data layout, and target traits
- `ExecutionDomain` is a declaration property, not a lexical IR block
- the architecture must be ready for one HIR module to lower into more than one
  target-specific LIR product in the future
- current implementation may remain host-only while these ownership boundaries
  are introduced

## Non-Goals

- no full host/device implementation yet
- no GPU backend bring-up yet
- no cross-domain runtime/link layer yet
- no silent continuation of idea 41 work if the current slice depends on the old
  raw-triple architecture staying in place

## Step 1. Inventory Target Ownership

Goal: produce the exact list of places that own, derive, or reinterpret target
triple, data layout, and ABI traits today.

Actions:

- inventory CLI target selection and propagation
- inventory preprocessor target-triple use
- inventory HIR/LIR ownership of target metadata
- inventory backend target parsing and data-layout derivation
- inventory helper functions that should become target-profile trait queries
- inventory tests with hard-coded target/layout literals

Completion Check:

- a concrete file-by-file ownership map exists for target profile data

## Step 2. Introduce Shared `TargetProfile`

Goal: replace raw target-string plumbing with a canonical target-profile model.

Actions:

- add a shared `TargetProfile` type and canonical resolver
- include triple, data layout, and target/ABI traits in that type
- thread the profile through the main compile entry points
- convert at least one target-trait query cluster away from raw triple/layout
  string checks
- use backend subset coverage such as
  `ctest --test-dir build -L backend --output-on-failure` plus focused
  frontend/codegen tests as the required gate for Step 2 slices

Completion Check:

- new code can obtain canonical target/data-layout information from one shared
  profile object

## Step 3. Preserve `ExecutionDomain`

Goal: introduce declaration-level execution-domain ownership without changing
current host-only behavior.

Actions:

- add execution-domain metadata to frontend/HIR-facing declaration structures
- default all current code paths to `Host`
- define the semantic model for future `Host`, `Device`, and `HostDevice`
- keep existing emitted output stable while the metadata is threaded through
- use backend subset coverage such as
  `ctest --test-dir build -L backend --output-on-failure` and relevant
  frontend/codegen unit coverage as the required gate for Step 3 slices

Completion Check:

- HIR can represent execution-domain ownership explicitly even though only the
  host path is active

## Step 4. Prepare `HIR -> LIR` Split Lowering

Goal: separate "lower one target-specific module" from "choose how many target
products to emit" without enabling full multi-target output yet.

Actions:

- refactor lowering boundaries so the architecture no longer hardcodes "one HIR
  module always becomes exactly one LIR module forever"
- keep the public behavior host-only for now
- make per-target module metadata come from `TargetProfile` ownership
- validate with focused frontend/codegen/backend coverage

Completion Check:

- the lowering architecture is ready for future host/device splitting without
  reworking the entire HIR/LIR ownership model

## Step 5. Realign Test Ownership

Goal: reduce accidental dependence on ad hoc per-file target/layout literals and
clarify which tests assert target-neutral contracts versus target-specific
emission.

Actions:

- centralize target/layout fixtures in tests where practical
- reduce direct hard-coded module target/layout duplication
- document which tests are route oracles and which test real emitters/contracts
- leave idea 41 in a good state to resume after this plan completes

Completion Check:

- tests stop using raw target/layout literals as a scattered pseudo-architecture
  layer

## Step 6. Final Validation

Goal: prove the target-profile foundation is stable before resuming idea 41.

Actions:

- use only backend-focused validation such as
  `ctest --test-dir build -L backend --output-on-failure` as the default
  regression loop during plan execution when backend surfaces change
- pair backend subset runs with relevant frontend/codegen tests when target
  propagation changes
- confirm current host-only outputs stay stable under the new target-profile
  ownership path
- confirm idea 41 can resume on top of the new architecture without depending
  on scattered raw target/layout plumbing

Completion Check:

- backend-focused and relevant frontend/codegen validation pass for the active
  slice, and the new foundation is ready for follow-on plans
