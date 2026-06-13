# 242 Phase E5 Route 6 scalar call-use source identity row follow-up

## Goal

Prove and implement one Route 6 scalar call-use source identity row or reader
that can consume a selected call argument/result source fact while retaining
prepared call plans, `ConsumedPlans`, public fallback, target-local call
policy, diagnostics, wrapper output, expected strings, and baselines.

This is a narrow successor idea from the E5 gate artifact:

- `docs/bir_prealloc_fusion/phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

## Why This Exists

The E5 gate found that no whole `PreparedBirModule` field group and no whole
`PreparedFunctionLookups` group is ready for deletion, privatization, or
aggregate replacement. It did identify Route 6 scalar call-use source identity
as a possible one-reader or one-row adapter candidate.

Closed idea 238 is prerequisite-complete only for the x86 Route 6 scalar `i32`
route-debug / `ConsumedPlans` compatibility boundary. That evidence does not
prove broad x86 call-wrapper migration, riscv readiness, route-wide wrapper
convergence, call-plan group deletion, or aggregate prepared retirement.

## In Scope

- One selected Route 6 scalar call-use source identity row or reader for a
  call argument/result role.
- Consumption of a selected scalar source agreement fact through an
  agreement-gated adapter.
- Retention of prepared call plans, `ConsumedPlans`, public prepared
  call/debug fallback, direct-call/helper-oracle families, target-local ABI
  placement, call-wrapper policy, helper/carrier protocols, wrapper output,
  expected strings, and baselines.
- Fail-closed behavior for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases.
- Nearby same-feature scalar call-use proof beyond one named fixture.

## Out Of Scope

- Broad x86 call-wrapper migration, route-wide x86 readiness, riscv readiness,
  or cross-target wrapper convergence.
- Whole `call_plans`, `PreparedFunctionLookups`, or `PreparedBirModule`
  deletion, privatization, or aggregate replacement.
- Draft 155 opening, broad rewrite, or broad successor plan.
- Moving ABI placement, frame layout, register allocation, call-wrapper
  policy, helper/carrier protocol, result lanes, outgoing stack layout,
  formatting, instruction selection, emission policy, or wrapper output into
  Route 6 or target-neutral BIR ownership.
- Expected-string rewrites, helper renames, supported-path downgrades,
  unsupported relabeling, wrapper-output relabeling, timeout masking, or
  baseline refreshes as proof.

## Acceptance Criteria

- Exactly one Route 6 scalar call-use source identity row or reader is named
  and routed through an agreement-gated adapter.
- The positive path consumes the selected scalar source agreement fact instead
  of re-deriving or testcase-matching the source locally.
- Prepared call plans, `ConsumedPlans`, public prepared call/debug fallback,
  direct-call/helper-oracle compatibility, and target-local call-wrapper
  policy remain authoritative for all fallback and policy-sensitive paths.
- Prepared printer/debug output, route-debug output, helper-oracle names and
  status labels, wrapper output, expected strings, supported-path contracts,
  and baselines remain byte-stable unless a separately approved source idea
  changes that contract.
- The implementation does not generalize idea 238 beyond its narrow x86 Route
  6 scalar `i32` route-debug / `ConsumedPlans` compatibility evidence.

## Required Proof Matrix

The implementation is not acceptance-ready until proof covers:

- the selected positive Route 6 scalar call-use source identity row or reader;
- absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where relevant;
- unchanged prepared call plans, `ConsumedPlans`, public call/debug fallback,
  direct-call/helper-oracle behavior, and status labels;
- unchanged route-debug output, wrapper output, expected strings, supported
  path status, and baselines;
- nearby same-feature scalar call-use cases proving the adapter is semantic and
  not tied to one named fixture;
- target-output no-change proof for adjacent ABI, frame, register, stack,
  helper/carrier, result-lane, outgoing-stack, formatting, emission, and
  wrapper surfaces.

## Reviewer Reject Signals

- The change claims or implies broad x86 call-wrapper migration, route-wide x86
  readiness, riscv readiness, cross-target wrapper convergence, call-plan group
  deletion, or aggregate prepared retirement.
- The implementation treats idea 238 as proof beyond x86 Route 6 scalar `i32`
  route-debug / `ConsumedPlans` compatibility.
- The implementation moves ABI placement, frame/register policy, call-wrapper
  policy, helper/carrier protocol, result lanes, outgoing stack layout,
  formatting, instruction selection, emission policy, or wrapper output into
  Route 6 or target-neutral BIR ownership.
- The adapter is a facade rename or public API hiding exercise that preserves
  the old failure mode.
- The patch rewrites expectations, helper names, supported-path status,
  wrapper output, or baselines instead of preserving behavior.
- The change downgrades supported tests to unsupported or weakens prepared
  call plans, `ConsumedPlans`, public fallback, diagnostic/oracle,
  debug/printer, wrapper, or helper contracts.
- The implementation adds named-case shortcuts, fixture-shaped matching, or
  special handling for one known scalar call testcase instead of consuming a
  real semantic source agreement fact.
