Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Mixed Edge Boundaries

# Current Packet

## Just Finished

Lifecycle activation created the runbook for Step 1.

## Suggested Next

Execute Step 1: audit the five representative mixed edge cases, record their
current diagnostics or runtime results, and choose the first implementation
packet.

## Watchouts

- Do not route vararg or `va_arg` cases back into this plan.
- Do not hard-code testcase names or source patterns.
- Split upstream missing prepared semantics into a separate idea instead of
  repairing them in RV64 object emission.

## Proof

Lifecycle-only activation; run `git diff --check -- plan.md todo.md`.
