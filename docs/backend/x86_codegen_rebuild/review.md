# X86 Codegen Rebuild Draft Review

## Scope Reviewed

This review covers the full stage-3 draft tree under
`docs/backend/x86_codegen_rebuild/` against the replacement manifest and route
constraints from:

- `docs/backend/x86_codegen_rebuild_plan.md`
- `docs/backend/x86_codegen_rebuild_handoff.md`

## Manifest Completeness

Review result: pass.

- the draft tree contains the required top-level `index.md`, `layout.md`, and
  this `review.md`
- every mandatory replacement header draft exists in the reviewed bucket where
  stage 2 declared it
- every mandatory replacement implementation draft exists in the reviewed
  bucket where stage 2 declared it
- the tree keeps the reviewed bucket structure: `api`, `core`, `abi`,
  `module`, `lowering`, `prepared`, and `debug`

## Dependency Direction Review

Review result: coherent.

- `api` is now a narrow public boundary that hands off to `module` instead of
  republishing the old `x86_codegen.hpp` mixed helper surface
- `core` owns shared types and output services rather than broad dispatch or
  prepared state
- `abi` owns stable machine-policy facts without taking over module or
  prepared routing
- `module` owns whole-module orchestration while delegating data emission to
  `module_data_emit`
- `lowering` names canonical owners for frame, call, return, memory,
  comparison, scalar, float, and atomics/intrinsics behavior
- `prepared` is written as a thin query/dispatch adapter over canonical seams
  instead of a parallel lowering stack
- `debug` is explicitly observational and does not claim lowering ownership

## Prepared-Route Constraint Review

Review result: pass with clear guardrails.

- prepared adapters now depend on `frame_lowering`, `call_lowering`,
  `memory_lowering`, and `comparison_lowering` instead of owning those
  semantics locally
- `prepared_fast_path_operands` is constrained to translate prepared facts
  into canonical queries rather than rebuild local-slot policy
- `prepared_fast_path_dispatch` owns admission and fallback only; it does not
  claim stack-layout, call-lane, or predicate semantics
- `prepared_route_debug` explains route decisions without becoming a helper
  bucket for code emission

## Boundary Review

Review result: pass.

- the draft tree continues to exclude the legacy `peephole/` subtree from
  claimed ownership
- the replacement contracts separate module-data emission from route selection
- the long-double/x87 compatibility pressure is isolated inside
  `float_lowering` instead of leaking into prepared or module ownership

## Remaining Implementation Risks

- stage 4 must preserve the documented dependency direction instead of
  reintroducing convenience includes or helper shortcuts that collapse the
  seams again
- prepared implementation work must reject testcase-shaped matcher growth when
  a shape really belongs in canonical lowering
- the draft set names ownership clearly, but the later implementation stage
  still needs disciplined API design so the seams stay narrow in real code

## Readiness Verdict

The stage-3 draft set is coherent enough to drive stage-4 implementation
conversion. The reviewed manifest is complete, the ownership graph is legible,
and the prepared-route adapters are constrained to consume canonical seams
instead of recreating the original hidden-lowering problem.
