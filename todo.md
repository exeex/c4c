Status: Active
Source Idea Path: ideas/open/134_shared_select_chain_same_block_dependency_queries.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Local Dependency Queries

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/134_shared_select_chain_same_block_dependency_queries.md`.

## Suggested Next

Start Step 1 by inventorying AArch64-local select-chain direct-global and
same-block scalar/constant materialization checks, then record the first
concrete shared-query candidate here.

## Watchouts

- Do not add named-case select-chain or direct-global shortcuts.
- Do not move AArch64 emission, hazard, scratch, or materialization-order
  policy into shared code.
- Do not weaken test expectations or mark supported paths unsupported.
- Keep matching `test_before.log` and `test_after.log` for backend proof.

## Proof

Lifecycle-only activation; no build or test run required.
