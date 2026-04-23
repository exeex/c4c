# Backend C-Testsuite Failure-Cluster Replan Umbrella

Status: Open
Created: 2026-04-18
Last-Updated: 2026-04-20

## Intent

Idea 57 is the umbrella source note for rebuilding the x86 backend capability
roadmap from current `backend` c-testsuite reality instead of from contract
shape alone.

The previous decomposition over-weighted the presence of prepared-handoff
fields and boundary tests. It did not stay grounded in the actual failure set
from:

`ctest --test-dir build -j8 -R backend --output-on-failure`

The current failure picture is broader than "prepared handoff exists or not".
A large slice still fails before x86 prepared consumption, several x86 emit
lanes remain bounded to minimal special cases, and a small set has already
crossed from unsupported capability into correctness bugs.

This umbrella exists to keep those ownership boundaries explicit.

## Current Failure Clusters This Umbrella Covers

The replanned series is grounded in these current clusters:

1. `semantic lir_to_bir` lowering gaps before x86 prepared emission
   Current size: 41 failures.
2. minimal single-block terminator / branch emission support
   Current size: 34 failures.
3. minimal direct-immediate scalar return expression support
   Current size: 30 failures.
4. single-function prepared-module restriction
   Current size: 18 failures.
5. authoritative prepared short-circuit handoff misses
   Current size: 6 failures.
6. authoritative prepared call-bundle handoff misses
   Current size: 5 failures.
7. authoritative prepared guard-chain handoff misses
   Current size: 4 failures.
8. runtime correctness failures after codegen succeeds
   Current size: 2 failures.

## The Decomposition

This umbrella is now split into six concrete source ideas:

1. [58_semantic_lir_to_bir_gap_closure_for_x86_backend.md](/workspaces/c4c/ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md)
   Owns current failures that stop in semantic lowering before prepared x86
   emission begins.
2. [59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md](/workspaces/c4c/ideas/open/59_cfg_contract_consumption_for_short_circuit_and_guard_chain.md)
   Owns current prepared control-flow handoff misses: short-circuit and
   guard-chain families.
3. [60_scalar_expression_and_terminator_selection_for_x86_backend.md](/workspaces/c4c/ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md)
   Owns current minimal single-block return and direct-immediate-only scalar
   return restrictions.
4. [61_call_bundle_and_multi_function_prepared_module_consumption.md](/workspaces/c4c/ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md)
   Owns current call-bundle and single-function prepared-module restrictions.
5. [62_stack_addressing_and_dynamic_local_access_for_x86_backend.md](/workspaces/c4c/ideas/open/62_stack_addressing_and_dynamic_local_access_for_x86_backend.md)
   Owns the stack/addressing portion of the current semantic-lowering gap,
   especially dynamic local/member/array access families.
6. [63_x86_backend_runtime_correctness_regressions.md](/workspaces/c4c/ideas/open/63_x86_backend_runtime_correctness_regressions.md)
   Owns cases that already compile and run far enough to expose wrong behavior
   instead of explicit unsupported-capability diagnostics.

## Ownership Rules

These ideas are intended to be mutually readable, but not vague:

- idea 58 owns upstream lowering blockers that prevent prepared-x86 handoff
  from existing at all
- idea 59 owns prepared CFG contract consumption gaps once lowering reaches
  x86
- idea 60 owns generic scalar expression and terminator selection after the
  needed CFG/value facts exist
- idea 61 owns call-bundle and cross-function prepared-module consumption
- idea 62 owns stack/addressing semantics where the right value or memory
  meaning is still not made canonical enough for x86 to consume generally
- idea 63 owns correctness bugs and should not be used to hide capability gaps

If a testcase fails before prepared emission exists, it belongs in idea 58 or
idea 62, not idea 60 or 61.

If a testcase reaches x86 prepared emission but fails because x86 still only
accepts tiny matcher-shaped programs, it belongs in idea 59, 60, or 61.

If a testcase reaches runtime and produces the wrong behavior, it belongs in
idea 63 even when the root cause may later trace back into another leaf.

## Suggested Ordering

The default route is:

1. idea 58
2. idea 62
3. idea 59
4. idea 61
5. idea 60
6. idea 63

The exact execution order can interleave ideas 59, 60, and 61 where shared
infrastructure justifies it, but the series should stay failure-cluster-driven
instead of returning to a vague "more x86 support" packet.

## Acceptance Standard For Future Plans

Future plans under this umbrella should prove progress against the owned failed
cases named in the leaf source ideas, not only against narrow boundary tests or
contract-field presence.

No leaf should be considered complete merely because a prepared contract struct
exists. Completion means the owned c-testsuite failure families are either
repaired or intentionally moved to a better-fitting source idea with an updated
durable note.

## What This Umbrella Does Not Own

This idea is not itself the next execution target.

It does not justify activating one large umbrella plan.

It does not accept testcase-shaped special cases as completion.

Activate a leaf idea whose owned failed-case families match the next intended
repair slice.
