Status: Active
Source Idea Path: ideas/open/181_phase_d_mir_consumer_bir_view_switch_plan.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Order the Migration Ladder

# Current Packet

## Just Finished

Step 4 from `plan.md` is complete. Updated
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` with an ordered
migration ladder and bounded follow-up idea summaries for route-backed
consumer groups. Each rung names the consumer group, BIR route prerequisite or
view, prepared oracle/fallback boundary, out-of-scope target policy, and proof
recommendation, with aggregate contraction, target-policy helpers, printer/debug
oracles, x86/riscv target wrappers, and return-chain work deferred behind their
prerequisites.

## Suggested Next

Execute Step 5 from `plan.md`: finalize the Phase D artifact and lifecycle
notes by checking the artifact against the source idea completion criteria,
Phase A-C/route closure links, return-chain exclusion, prepared API
non-deletion claim, and artifact-only proof record.

## Watchouts

- Phase D is analysis-only; do not edit backend implementation files.
- Do not turn `PreparedFunctionLookups` into a BIR-owned aggregate clone or
  claim prepared API deletion from selected consumer switches.
- Step 5 should verify the Step 4 ladder remains directly consumable by
  follow-up ideas without creating those `ideas/open/` files unless the
  supervisor explicitly delegates lifecycle work.
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
`docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` contains a Step 4
ordered migration ladder and follow-up summaries naming consumer group, BIR
route prerequisite/view, prepared oracle/fallback boundary, out-of-scope target
policy, and proof recommendation; blocked/deferred items are behind
prerequisites; and the section makes no prepared API deletion claim. No
`test_after.log` was produced because the delegated proof explicitly required
no build/test.
