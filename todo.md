Status: Active
Source Idea Path: ideas/open/78_aarch64_edge_copy_typed_stack_source_prepared_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Edge-Copy Publication Inputs

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: inspect `dispatch_edge_copies.cpp` call sites and locate where
`PreparedEdgePublication` can feed
`prepare_same_width_i32_stack_source_publication`.

## Watchouts

- Do not thread `PreparedEdgePublication` into `dispatch_publication.cpp` just
  to satisfy this route.
- Do not add named testcase shortcuts or weaken expected supported behavior.
- Keep AArch64 load/copy/register emission target-local.

## Proof

Not run; lifecycle activation only.
