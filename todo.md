Status: Active
Source Idea Path: ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define BIR View and Adapter Boundaries

# Current Packet

## Just Finished

Step 3 from `plan.md` is complete. Updated
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` with a concrete
boundary section that defines route-backed BIR views and adapters for Routes
1-7, prepared fallback/oracle surfaces, durable target-policy non-migration
surfaces, and return-chain as a no-route blocker.

## Suggested Next

Execute Step 4 from `plan.md`: turn the Step 2 map and Step 3 boundaries into
an ordered migration ladder with bounded follow-up idea summaries, proof
recommendations, prepared oracle boundaries, and out-of-scope target policy for
each proposed consumer switch.

## Watchouts

- Phase D is analysis-only; do not edit backend implementation files.
- Do not turn `PreparedFunctionLookups` into a BIR-owned aggregate clone.
- Step 4 should order migrations by consumer group and route-backed view, not by
  broad prepared aggregate removal.
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
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` contains a Step 3
BIR view and adapter boundary section naming route-backed BIR views for Routes
1-7 where applicable, prepared fallback/oracle boundaries, durable
target/layout/codegen policy non-migration boundaries, x86/riscv
interface-level boundaries, and return-chain as a no-route blocker. No
`test_after.log` was produced because the delegated proof explicitly required
no build/test.
