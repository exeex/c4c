Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Inventory And 594 Handoff

# Current Packet

## Just Finished

Completed Step 5 for `plan.md`: recorded lifecycle-ready closure inventory for
idea 593 and the 594 handoff status without moving or closing source ideas.

Closure answers for
`ideas/open/593_rv64_branch_stack_source_freshness_consumption.md`:

1. Migrated RV64 consumer path: the prepared RV64 object-emission fused pointer
   conditional branch path for a stack-slot `Lhs` operand. The route now
   requires a selected shared `PreparedValueFreshnessUseKind::BranchStackLoadSource`
   / `BranchStackSlot` authority for the same prepared source value, exact
   branch block, and terminator instruction index before emitting the stack load
   into the RV64 `Lhs` scratch register and branching.
2. Remaining unwired branch stack-source shapes: pointer `Rhs` remains unwired
   because the producer/collector currently records it as inventory-only with
   `policy=none` / `status=missing_policy`; aggregate-adjacent branch
   stack-source consumers and scalar-condition-register branch shapes were not
   selected for this narrow 593 migration; string assembly emission and other
   target emission paths remain outside this slice. These need a follow-up only
   where shared producer authority exists or the producer contract is repaired.
3. RV64 freshness fallback status: no selected RV64 path in the 593 slice is
   allowed to rely on stack-home-only, frame-slot-only, aggregate-lane-only,
   clobber-only, register-only, or operand-shape-only evidence as freshness.
   The migrated `Lhs` route fails closed unless selected shared authority
   matches the exact branch use; layout and clobber facts remain separate
   support facts, not freshness.
4. Missing shared producer fact: yes. Pointer `Rhs` exposed a missing producer
   policy/fact and should return to the 592 family rather than be patched by an
   RV64 fallback. No target-local RV64 freshness manufacture is acceptable for
   that gap.
5. AArch64/x86 follow-up: no new AArch64 or x86 implementation follow-up was
   exposed by this RV64-only slice. Those targets remain intentionally deferred
   under the source idea's out-of-scope boundary.
6. Prepared MIR view readiness: RV64 behavior is stable enough for the later
   Prepared MIR view line to require selected shared branch stack-source
   authority for the migrated `Lhs` backend input, but not stable enough to
   treat every RV64 branch stack-source shape as fully wired until the `Rhs`
   producer/consumer handoff is resolved.
7. Exact 594 gap: 594 should take over the pointer `Rhs` branch stack-source
   consume-side gap, starting by checking whether the missing `Rhs`
   producer/collector policy has been supplied by the 592 family; if not, 594
   should block on producer repair instead of adding an RV64 fallback.
8. Existing 594 file: `ideas/open/594_rv64_branch_stack_source_consumption_followup_from_593.md`
   already exists and is the correct handoff slot for the next RV64
   consume-side gap.

Validation summary to carry into closure review: Step 4 backend proof passed
346/346 using the supervisor-selected backend subset; regression guards passed;
hook-managed full-suite baselines after the code commits were accepted at
3375/3375.

## Suggested Next

Supervisor review should decide whether the Step 5 inventory is acceptable,
then hand lifecycle closure to plan-owner. Plan-owner, not executor, should move
or close idea 593 if the review accepts this inventory.

## Watchouts

- Do not start implementation for 594 before 593 closes with this concrete
  closure-note handoff accepted by supervisor/plan-owner.
- Do not use RV64 target-local stack-home, frame-slot, aggregate-lane, clobber,
  register, or operand-shape evidence as freshness.
- If a producer fact promised by 592 is missing, record that as a blocker for
  the 592 family instead of manufacturing fallback freshness in RV64.
- Pointer `Rhs` is the concrete 594 gap, but current evidence says it is still
  gated by missing producer policy/fact (`policy=none`,
  `status=missing_policy`). 594 should block on producer repair if that remains
  true.
- Step 4 added a narrow RV64-local allowance for the older GPR-only fused
  pointer publication rejection only when stack-slot `Lhs` selected freshness
  has already passed and `Rhs` remains register-compatible or null. Do not
  broaden that allowance to pointer `Rhs`, scalar condition register branches,
  string assembly emission, aggregate-adjacent consumers, AArch64, or x86.

## Proof

Proof Command: none; closure inventory only.

No new proof was run for this documentation-only Step 5 packet. Carried-forward
proof state: Step 4 backend proof passed 346/346 with canonical log
`test_after.log`; regression guards passed; hook-managed full-suite baselines
after the code commits were accepted at 3375/3375.
