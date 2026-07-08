# Uniform Target Register Identity Policy Runbook

Status: Active
Source Idea: ideas/open/586_uniform_target_register_identity_policy.md
Activated from: ideas/open/586_uniform_target_register_identity_policy.md

## Purpose

Make target ABI physical register identity publication a shared
prepared/prealloc policy for RV64, AArch64, and x86 where the ABI placement has
a stable physical identity.

## Goal

Extend `src/backend/prealloc/target_register_profile.*` so supported ABI
argument/result placements can publish `PreparedTargetRegisterIdentity` through
one shared target-register policy surface.

## Core Rule

Do not change semantic ABI classification, value freshness authority,
preservation fallback, move-bundle authority, broad backend lowering, or test
expectations as part of this runbook.

## Read First

- Source idea:
  `ideas/open/586_uniform_target_register_identity_policy.md`
- Research parent:
  `docs/target_abi_contract_research/`
- Target register policy:
  `src/backend/prealloc/target_register_profile.*`
- Prepared/prealloc placement and identity types.
- Current RV64 ABI placement identity path:
  `target_register_identity_for_abi_register_placement(...)`
- AArch64 and x86 prepared/object backend call argument and result consumers.

## Current Scope

- Audit current target-register policy helpers and identity publication.
- Preserve `PreparedRegisterPlacement` as the allocation-policy placement.
- Use `PreparedTargetRegisterIdentity` only for concrete physical ABI register
  identity where it is stable and meaningful.
- Add focused proof for ABI argument/result identity publication across RV64,
  AArch64, and x86 where supported.
- Document any ABI placement shape that intentionally remains identity-less or
  fail-closed.

## Non-Goals

- Do not change `CallArgAbiInfo`, `CallResultAbiInfo`, or target triple
  selection.
- Do not implement prepared value freshness authority, producer
  rematerialization precedence, `PriorPreservation` semantics, or stale-home
  invalidation.
- Do not rework move-bundle source authority or stack-destination fan-in
  semantics.
- Do not broadly rewrite RV64, AArch64, or x86 call lowering.
- Do not replace final target instruction rendering or assembler register
  spelling.
- Do not claim progress through expectation rewrites, unsupported-marker
  changes, allowlist edits, or runtime-comparison changes.

## Working Model

- `target_register_profile` owns target ABI register pools, ABI placements,
  caller/callee-save pools, and physical identity publication for stable ABI
  placement identities.
- `PreparedRegisterPlacement` remains target-profile-relative placement policy.
- `PreparedTargetRegisterIdentity` is the concrete target physical identity
  consumed by prepared/prealloc or backend code that needs physical identity.
- Unsupported or unstable identity publication must fail closed or be
  explicitly documented; it must not fall back to backend-local rediscovery.

## Execution Rules

- Keep routine packet progress and proof notes in `todo.md`.
- Prefer shared target-register-profile helpers over target-local string or
  spelling checks.
- Preserve existing RV64 behavior while extending the same policy surface to
  AArch64 and x86.
- Add focused tests near the existing prealloc/backend-BIR target register
  placement coverage.
- Escalate to supervisor or reviewer if the implementation route starts
  changing semantic ABI facts, value freshness, preservation, move-bundle
  authority, or broad backend lowering.
- For code-changing steps, run a fresh build or compile proof plus the focused
  tests selected by the supervisor.

## Ordered Steps

### Step 1: Audit Current Target Register Identity Policy

Goal: Identify the exact shared and target-local surfaces that currently
publish or rediscover ABI physical register identity.

Primary targets:
- `src/backend/prealloc/target_register_profile.*`
- Prepared register placement and target identity type definitions.
- RV64, AArch64, and x86 prepared/object backend consumers that need ABI
  argument/result physical identity.

Actions:
- Inspect existing ABI argument/result register pool helpers for RV64, AArch64,
  and x86.
- Trace the current RV64
  `target_register_identity_for_abi_register_placement(...)` behavior.
- Find AArch64 and x86 consumers that currently rely on placement conversion,
  register names, or backend-local rendering instead of shared physical
  identity.
- Record any ABI placement shapes that should remain identity-less because the
  physical identity is not stable, meaningful, or supported.

Completion check:
- `todo.md` names the shared helper surfaces, the target-local consumers, and
  the candidate extension points for AArch64 and x86 without changing
  implementation files.

### Step 2: Extend Shared Identity Publication

Goal: Make `target_register_profile` publish physical identity for supported
RV64, AArch64, and x86 ABI argument/result placements through one shared
surface.

Primary target:
- `src/backend/prealloc/target_register_profile.*`

Actions:
- Preserve the existing RV64 ABI placement identity behavior.
- Add AArch64 and x86 ABI placement identity mapping where the target ABI
  register identity is stable.
- Fail closed or return an explicit unsupported/no-identity result for
  unstable or unsupported placement shapes.
- Keep the mapping based on target-register policy facts, not backend-local
  string matching.

Completion check:
- Supported RV64, AArch64, and x86 ABI argument/result placements can be
  queried for `PreparedTargetRegisterIdentity` through the same target-profile
  policy surface, and unsupported shapes do not silently fabricate identity.

### Step 3: Adapt Consumers Without Broad Lowering Rewrites

Goal: Route narrow prepared/prealloc or backend consumers that need ABI
physical identity through the shared publication surface.

Actions:
- Replace narrow backend-local rediscovery at identified call argument/result
  consumers with the shared identity helper where appropriate.
- Preserve existing prepared register names, placements, call plans, value
  homes, move bundles, and printers unless a small adaptation is required to
  consume the shared identity.
- Do not change semantic ABI classification or call-lowering structure.

Completion check:
- Consumers that require physical ABI identity use the shared
  `PreparedTargetRegisterIdentity` path or explicitly document why they remain
  identity-less/fail-closed.

### Step 4: Add Focused Identity Publication Tests

Goal: Prove ABI argument/result placement identity publication for every
supported target family.

Actions:
- Add or extend focused prealloc/backend-BIR tests for RV64 ABI
  argument/result placements.
- Add corresponding AArch64 and x86 coverage where the target has stable
  physical argument/result identities.
- Assert stable `PreparedTargetRegisterIdentity` facts rather than only string
  register spellings.
- Cover at least one unsupported or identity-less shape if the implementation
  introduces an explicit fail-closed path.

Completion check:
- Focused tests prove supported RV64, AArch64, and x86 ABI placements publish
  the expected physical identities through the shared policy surface.

### Step 5: Validate And Inventory Remaining Unsupported Shapes

Goal: Confirm the narrow identity policy slice is complete without drifting
into freshness or backend-lowering work.

Actions:
- Run the supervisor-selected focused proof and any required build or broader
  backend/prealloc checks.
- Confirm existing RV64 ABI identity behavior still passes.
- Confirm existing prepared call contract and target register placement tests
  still pass.
- Record remaining unsupported or intentionally identity-less placement shapes
  in `todo.md` for closure consideration.

Completion check:
- Proof is fresh, root-level canonical logs are updated as directed by the
  supervisor, and `todo.md` contains a concise unsupported-shape inventory
  suitable for final lifecycle review.
