# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The `match` Structural-Dispatch Topology
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Plan review corrected Step 2.1 scope. Repeated executor packets and the
semantic `match` body from `00204.c` show the target block
`%t27 = bir.ne i8 %t26, 0` -> `%t28 = bir.xor i1 %t27, 1` ->
`%t29 = bir.zext i1 %t28 to i32` -> `%t30 = bir.ne i32 %t29, 0` ->
`bir.cond_br i1 %t30, block_67, block_68` is already covered by the owned x86
bool/cast helpers. The live blocker is the larger `match` topology around that
block: the guard sits downstream of an earlier loop plus short-circuit/phi
guard and upstream of a parameter-address store leaf, so the whole function
still falls out of the existing bounded local-slot structural-dispatch lanes.
`plan.md` now reflects that corrected route.

## Suggested Next

Reduce `match` to the smallest owned structural-dispatch reproducer that still
fails while preserving the enclosing topology that matters:
the earlier loop, the short-circuit/phi guard, the second compare-driven
branch in one function, and the downstream parameter-address store leaf. Use
that reproducer to prove whether the missing ownership is inside the bounded
local-slot / compare-driven dispatcher or in a shared prepared control-flow
fact before extending any consumer seam.

## Watchouts

- Do not claim Step 2.1 progress by adding a named `match` matcher. The owned
  bool/cast machinery already handles the raw `%t27/%t28/%t29/%t30` chain.
- Do not broaden the raw `i8` short-circuit or continuation fallback to force
  `match` through. The guard-lane and short-circuit boundary tests still
  protect that rejection.
- Treat the blocker as structural: loop + short-circuit/phi guard + second
  compare-driven branch + parameter-address store leaf. Do not collapse the
  packet back into the terminal `ne i32 ..., 0` fragment.
- Float/HFA local-slot work, unrelated `i64` local-slot work, and later
  call-boundary routing remain out of scope for this corrected Step 2.1.
- This was a lifecycle repair, not an implementation landing. Fresh proof
  still belongs to the next executor packet.

## Proof

Lifecycle repair only. No new proof run was performed in this packet. The
current evidence to carry forward is the repeated delegated proof result:
`backend_x86_handoff_boundary` passes, while
`c_testsuite_x86_backend_src_00204_c` still fails with the same top-level x86
minimal-return/guard restriction after the standalone bool/cast fragment is
confirmed to already be covered. Canonical proof log remains `test_after.log`
from the last executor packet.
