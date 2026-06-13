# 238 Phase E4 x86 Route 6 scalar i32 route-debug ConsumedPlans compatibility follow-up

## Goal

Converge one x86 Route 6 scalar `i32` argument-source route-debug row so it can
consume the proven Route 6 scalar source agreement fact while retaining
`ConsumedPlans` compatibility and all x86 target-local call-wrapper policy.

This idea is the single accepted Phase E4 follow-up from
`ideas/open/237_phase_e4_cross_target_wrapper_convergence_readiness.md`.

## Why This Exists

Closed idea
`ideas/closed/232_phase_e3_route6_x86_scalar_i32_argument_source_route_debug_follow_up.md`
proved the selected x86 Route 6 scalar `i32` argument-source route-debug row
can use Route 6 scalar source agreement while preserving `ConsumedPlans`
compatibility. Phase E4 classified that exact row as ready for one wrapper
convergence implementation idea.

The readiness is deliberately narrow. It does not prove broad x86 call-wrapper
migration, riscv readiness, route-wide wrapper convergence, prepared aggregate
retirement, or ownership transfer for ABI, frame, register, formatting,
emission, or call policy.

## In Scope

- One x86 Route 6 scalar `i32` argument-source route-debug row.
- Consumption of the Route 6 scalar source agreement fact accepted by closed
  idea `232`.
- Retention of x86 ABI placement, call-wrapper policy, route-debug row
  formatting, direct-call behavior, helper/carrier protocols, and wrapper
  output as target-local policy.
- Retention of prepared call plans, `ConsumedPlans`, public prepared call/debug
  fallback, and direct-call/helper-oracle families as compatibility adapters.
- Fail-closed behavior for absent, invalid, duplicate/conflict, mismatch,
  non-agreement, unsupported, public fallback, and policy-sensitive paths.
- Byte-stable route-debug output, wrapper output, helper-oracle behavior,
  expected strings, supported-path status, and baseline behavior.

## Out Of Scope

- Broad x86 call-wrapper, ABI, call lowering, prepared call printer/debug, or
  wrapper-family migration.
- Any riscv readiness claim or riscv-only adapter.
- Moving ABI, frame layout, register allocation, call-wrapper policy,
  formatting, instruction selection, emission, or wrapper output into Route 6
  or target-neutral BIR ownership.
- Deleting or weakening `ConsumedPlans`, prepared call plans, direct-call
  helpers, public prepared fallback, or helper-oracle families.
- Expected-string rewrites, helper renames, supported-path downgrades,
  unsupported relabeling, timeout masking, wrapper-output relabeling, or
  baseline refreshes as proof.
- Aggregate `PreparedFunctionLookups`, `PreparedBirModule`, draft 155, E5
  prepared aggregate retirement, or broad diagnostic/oracle replacement.

## Acceptance Criteria

- The selected positive x86 Route 6 scalar `i32` argument-source route-debug
  row consumes Route 6 scalar source agreement instead of re-deriving or
  testcase-matching the source locally.
- Existing x86 target-local policy remains authoritative for ABI placement,
  call-wrapper behavior, direct calls, helper/carrier protocols, route-debug
  formatting, and wrapper output.
- `ConsumedPlans`, prepared call plans, prepared call/debug fallback, and
  direct-call/helper-oracle compatibility remain intact for all fallback and
  non-agreement paths.
- The implementation proves nearby same-feature scalar source behavior rather
  than only one named fixture.
- No expected strings, helper names, supported-path contracts, wrapper output,
  or baselines are weakened to make the route appear converged.

## Required Proof Matrix

The implementation is not acceptance-ready until proof covers:

- positive Route 6 scalar `i32` source-agreement behavior;
- absent source facts;
- invalid source facts;
- duplicate/conflict source facts;
- mismatch and non-agreement behavior;
- public prepared fallback and `ConsumedPlans` compatibility;
- wrapper output stability;
- route-debug output stability;
- direct-call/helper-oracle behavior and status labels;
- expected-string stability;
- baseline-stability behavior without treating a baseline refresh as proof.

## Reviewer Reject Signals

- The change turns the accepted row into broad x86 call-wrapper migration,
  broad wrapper-family migration, or route-wide x86 readiness.
- The route claims or implies riscv readiness from x86-only repair evidence.
- The implementation rewrites expectations, helper names, supported-path
  status, wrapper output, or baselines instead of preserving behavior.
- The change downgrades supported tests to unsupported or weakens route-debug,
  helper-oracle, direct-call, public fallback, or `ConsumedPlans` contracts.
- The implementation adds named-case shortcuts, fixture-shaped matching, or
  special handling for one known `i32` testcase instead of consuming Route 6
  scalar source agreement semantically.
- ABI, call-wrapper policy, frame/register placement, formatting,
  instruction-selection, emission, or wrapper output moves into Route 6 or BIR
  ownership.
- `ConsumedPlans`, prepared call plans, prepared fallback, or helper-oracle
  compatibility is deleted, bypassed, or hidden behind a new abstraction name
  while preserving the old failure mode.
