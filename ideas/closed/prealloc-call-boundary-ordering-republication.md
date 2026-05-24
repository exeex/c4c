# Prealloc Call-Boundary Ordering And Republication Ownership

## Intent

Extend prealloc call-plan ownership so generic call-boundary move ordering,
preservation-home population, source/destination effect resources, and
republication intent are planned before target-specific call emission.

## Why This Exists

The first forward-migration wave moved call-boundary classification into
prealloc, but AArch64 call lowering still owns generic ordering and
republication mechanics around calls. x86 and RISC-V have the same before-call,
after-call, return-value, and preservation problems even though their ABI lanes
and final move instructions differ.

This idea focuses on ownership of ordering and intent, not target ABI emission.

## In Scope

- Move generic before-call, after-call, return-value, preservation, and
  republication ordering into prealloc call plans or adjacent call-plan helpers.
- Represent source and destination effect resources without AArch64 register
  spelling.
- Keep target-specific ABI lane assignment, variadic details, byval emission,
  indirect callee materialization, and machine moves behind AArch64 hooks.
- Update AArch64 call lowering to consume the extended plan while preserving
  current behavior.
- Add an x86 or RISC-V call-plan fixture or consumer proving the generic
  ordering model is reusable.

## Out Of Scope

- AArch64 instruction spelling, ABI lane policy, stack-pointer sequences,
  memory operand spelling, and final machine instruction construction.
- Changing target ABI classification rules, F128 carrier policy, variadic ABI
  handling, byval copy spelling, or actual machine move emission.
- Publication planning outside call-boundary republication intent.
- Broad rewrites of call lowering unrelated to ordering ownership.

## Acceptance Criteria

- Prealloc call plans own the generic ordering and republication-intent facts
  needed by current AArch64 call lowering.
- AArch64 call lowering still owns target ABI policy and final emission.
- Existing AArch64 call, byval, preserved-value, indirect-callee, and
  return-value behavior remains equivalent.
- At least one x86 or RISC-V fixture or compile path consumes the generic
  ordering model without AArch64 codegen dependencies.

## Proof Expectations

- Build proof for affected C++ targets.
- Focused AArch64 call tests covering before-call moves, after-call moves,
  preserved values, indirect callees, byval handling, and return values touched
  by the migration.
- One x86 or RISC-V call-plan unit or fixture demonstrating reuse of the
  generic ordering model.

## Reviewer Reject Signals

- The patch moves AArch64 ABI lane policy, concrete register names, variadic
  ABI details, F128 carriers, byval copy instructions, or final machine moves
  into prealloc.
- Existing supported AArch64 call tests are weakened, marked unsupported, or
  rewritten to fit the new plan.
- The new prealloc surface only describes one named failing call shape instead
  of generic call-boundary ordering.
- Republication remains effectively owned by AArch64 codegen behind a renamed
  helper, with no meaningful prealloc plan data.
- The x86/RISC-V proof is absent or depends on including AArch64 call-lowering
  headers.

## Closure Note

Closed after introducing neutral prealloc call-boundary effect records and
`plan_prepared_call_boundary_effects`, adding focused record-level coverage,
adapting AArch64 explicit before-call and after-call move paths through narrow
local adapters, and adding the x86-labeled
`backend_x86_call_boundary_effect_ordering` reuse proof without AArch64 codegen
dependencies.

Preservation-home population and preservation republication emission remain
target-local for this idea because their remaining behavior is tied to
AArch64 ordering, register aliasing, operand conversion, and final instruction
emission policy. Those target-policy-adjacent loops are not blockers for this
generic effect-plan boundary.
