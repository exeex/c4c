# Current Packet

Status: Active
Source Idea Path: ideas/open/723_pre_regalloc_value_constraint_carrier_research.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace existing constraint flow

## Just Finished

- Plan Step 1 documented the complete current `PreparedAllocationConstraint` flow in `docs/pre_regalloc_value_constraints/01_existing_constraint_flow.md`: stable BIR/liveness identity, the sole constraint construction site, target-policy derivation, allocator behavior, and observational test reads.
- The trace establishes two discontinuities without choosing a Step 2 architecture: no general named-value fixed/preferred request reaches `BirPreAlloc::run_regalloc`, and the allocator does not consume the constraint rows it publishes.

## Suggested Next

- Execute Plan Step 2 by comparing BIR, prepared semantic, liveness, and regalloc ownership and deciding whether a general carrier is valid.

## Watchouts

- `PreparedAllocationConstraint` is currently descriptive: preferred/forbidden pools duplicate policy independently used by assignment, while fixed fields are always absent. Step 2 must account for both ingress and enforcement rather than treating the existing row as an active control surface.

## Proof

- No build was required for this documentation-only packet.
- Ran repository `rg` checks for every cited constraint field, producer/consumer symbol, liveness path, register-group override, target pool helper, ABI helper, and test observation; all cited paths and symbols resolved.
