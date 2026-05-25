# Shared Call Plan Authority

## Goal

Move call-lowering decisions out of target-local AArch64 codegen and into a
shared BIR/prepared call-plan layer that all target backends can consume.

## Why This Exists

`src/backend/mir/aarch64/codegen/calls*.cpp` currently contains a large amount
of logic that is not inherently AArch64 instruction emission: call argument
source recovery, boundary move planning, byval aggregate handling, preservation
decisions, stack argument layout, and result publication. The reference backend
keeps the target-local `calls.rs` small by placing classification and generic
dispatch in shared backend layers. C4C should follow that layering principle
without copying the reference text-emitter design.

## In Scope

- Define a shared prepared call-plan API for caller-side call lowering.
- Represent argument placement, stack argument layout, sret/byval/variadic
  handling, call-boundary moves, preservation requirements, and result
  placement as prepared facts.
- Keep target ABI facts parameterized so AArch64, x86, and future targets can
  provide register sets, alignment, aggregate passing, variadic, and return
  rules without duplicating the generic planner.
- Migrate the first meaningful AArch64 call-lowering decision paths to consume
  the shared call plan.
- Add or update focused tests that prove behavior is preserved through the
  shared plan route.

## Out Of Scope

- Mechanical file merging of `calls*.cpp` without moving decision authority.
- Rewriting all target backends in one pass.
- Changing C call ABI semantics except where existing behavior is proven wrong.
- Dispatch cleanup outside call-specific paths.
- Assembly printer or encoder rewrites unrelated to call-plan consumption.

## Acceptance Criteria

- A shared BIR/prepared call-plan surface exists and owns at least one
  previously AArch64-local call decision family.
- AArch64 call emission consumes the shared plan for that family and keeps only
  target instruction emission in target-local code.
- The migrated path has build proof plus focused runtime or backend tests.
- No tests are weakened, marked unsupported, or converted to narrower
  expectation-only proof.
- The remaining target-local call code has a clearer boundary between
  decision-making and AArch64 machine instruction emission.

## Completion Notes

Closed after migrating callee-saved preservation boundary effects to shared
prepared call-plan authority. `PreparedCallBoundaryEffectEndpoint` now exposes
the endpoint facts needed for preservation storage, and AArch64 before/after
call preservation consumes `PreparedCallBoundaryEffectPlan` entries for
preservation population and republication instead of making the storage
decision from target-local call-plan iteration.

The AArch64 live-use gates for when a prepared preservation effect emits at
block entry or around a call remain target-local emission concerns. The
redundancy guard that checks prior preserved values also remains target-local,
but it is not the current-call preservation decision source.

Deferred follow-up boundaries for later ideas:

- Consolidating the split AArch64 `calls*.cpp` emission files belongs to the
  emission consolidation idea.
- Reducing broader dispatch responsibility belongs to the dispatch reduction
  idea.
- Additional call decision families, such as argument/result placement,
  byval/variadic details, and stack argument layout, should migrate only as
  separate shared-authority slices with focused proof.

Close proof used backend-scope regression guard logs generated with:
`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } > test_after.log 2>&1'`.
The matching close gate passed with 162/162 backend tests before and after, no
new failures, and no test-contract downgrades.

## Reviewer Reject Signals

- A patch mainly renames helpers or moves code between AArch64 files while the
  same call decisions remain target-local.
- A patch hard-codes AArch64-specific facts into the shared planner instead of
  exposing them through a target ABI policy.
- A patch makes only a named testcase pass through special-case matching rather
  than a general call-plan rule.
- A patch downgrades runtime, backend, or expectation contracts without
  explicit user approval.
- A patch claims shared call-plan progress while AArch64 emission still
  reconstructs stack/register argument placement from raw BIR shapes.
