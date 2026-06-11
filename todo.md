Status: Active
Source Idea Path: ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Map Prepared Consumer Families

# Current Packet

## Just Finished

Step 2 from `plan.md` is complete. Updated
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` with a concrete
consumer-family dependency map for live MIR/codegen reads of
`PreparedBirModule`, `PreparedFunctionLookups`, and domain `Prepared*` query
structs.

## Suggested Next

Execute Step 3 from `plan.md`: turn the route-backed rows from the Step 2 map
into explicit BIR view and adapter boundaries, while naming prepared fallbacks,
durable target-policy surfaces, and no-route blockers.

## Watchouts

- Phase D is analysis-only; do not edit backend implementation files.
- Do not turn `PreparedFunctionLookups` into a BIR-owned aggregate clone.
- Keep target/layout/codegen policy out of BIR views: call ABI placement,
  storage/home/move policy, memory operand formation, wide-value carrier
  layout, runtime helper protocols, and final machine-record spelling stay
  prepared/AArch64-owned or target-owned.
- Keep return-chain lookup as a no-route gap unless the separate owner/schema
  line is explicitly imported.
- x86 and riscv currently need only interface-level boundaries; do not create
  target-specific Phase D adapters before AArch64 route-view equivalence is
  proven.

## Proof

Docs-only analysis packet; no build/test required by the delegated proof. Local
verification checked that
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` contains a Step 2
consumer-family dependency map with the four classifications from `plan.md` and
covers AArch64 traversal/dispatch, calls, memory/addressing, value
materialization/publication, comparison/ALU, edge copies/control flow, wide
values/runtime helpers, future x86/riscv interfaces, and return-chain as a
no-route blocker.
