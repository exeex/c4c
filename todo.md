Status: Active
Source Idea Path: ideas/open/195_cross_target_route_view_reuse_beyond_x86_route6.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Produce the durable cross-target reuse table

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: created the durable cross-target route-view
reuse audit artifact at
`docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md`.

The artifact consolidates the Step 1 wrapper inventory, Step 2 readiness
classification, and Step 3 proof-route decisions into a citation-ready table
covering target, wrapper boundary, consumed fact/helper, required route view,
required agreement and fallback checks, retained target-local policy, readiness,
and proof route.

It preserves the key Step 3 conclusion: the existing x86 Route 6 scalar helper
is the only ready reuse point, and no additional x86 or riscv boundary has a
clear proof route yet. riscv status is explicit and remains not ready.

## Suggested Next

Start Step 5 by validating lifecycle and proof sufficiency for the audit: check
that only docs/lifecycle files changed, no implementation or expectation files
were touched, no named-case shortcuts or unsupported rewrites were used, and
the durable artifact gives the supervisor enough information to decide whether
idea 195 should close or spawn narrower implementation ideas.

## Watchouts

- The artifact intentionally does not claim riscv readiness.
- The future implementation boundaries are advisory only; the supervisor and
  plan owner still own whether to close idea 195 or create new ideas.
- Do not treat x86 Route 6 scalar helper evidence as proof for whole call plans,
  `ConsumedPlans`, `x86::prepared::Query`, x86 edge/memory wrappers, or any
  riscv wrapper.
- Any code-changing follow-up must remain one wrapper boundary wide and include
  build proof plus target-specific narrow tests and adjacent same-feature
  sanity cases.

## Proof

Lifecycle/docs-only packet. Used the Step 1 inventory from `git show
HEAD~2:todo.md`, the Step 2 classification from `git show HEAD~1:todo.md`, the
current Step 3 proof-route decision in `todo.md`, and targeted `rg`
confirmation for `find_consumed_scalar_i32_call_argument_source`,
`route6_find_call_argument_source`, riscv `consume_edge_publication_move_intent`,
and `prepare_same_width_i32_stack_source_publication`. No build or test proof
was required by the delegated packet, and no `test_after.log` was generated.
