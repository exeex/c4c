Status: Active
Source Idea Path: ideas/open/93_hir_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Public HIR Surface

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/93_hir_agent_index_header_hierarchy.md`.

## Suggested Next

Execute Step 1 in `plan.md`: inspect the public HIR facade, IR contract, and
printer/summary caller surface before moving private implementation indexes.

## Watchouts

- Preserve HIR behavior and testcase expectations.
- Do not split `hir_ir.hpp` merely for C++ interface purity.
- Do not create one header per HIR `.cpp`.
- Keep parser follow-up work from `ideas/open/94_parser_public_facade_pimpl_boundary.md`
  separate.

## Proof

Lifecycle-only activation; no build proof required.
