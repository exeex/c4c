# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics repaired the prepared
return move bundle blocker for `branch_join_immediate_then_xor`. Regalloc now
infers scalar return ABI authority when semantic BIR has a return type but no
explicit return ABI record, and the joined-branch immediate return-context
fixture refreshes its prepared value-location return bundle after mutating the
already-prepared join carrier/topology metadata.

## Suggested Next

Continue Step 4 at the later value-location blocker now exposed by the selected
proof: the selected x86 handoff subset advances to
`branch_join_global_then_xor`, where the prepared consumer rejects the route as
an unsupported global compare-join return context. Keep any repair on prepared
same-module global/value-location records; do not add expected-assembly or
function-name fallbacks.

## Watchouts

Do not turn the same-module global or control-flow route into a named-fixture
dispatcher. The scalar route is gated by supported same-module globals, `i32`
immediate stores, `i32` global loads without pointer-backed addresses, `i32`
subtraction, and prepared value-location authority. Step 4 should route
branches and labels through prepared label identity where available, reject or
surface missing and drifted label ids, and keep scalar rendering assumptions
explicit. The current route renders into a local buffer and only publishes
assembly after the whole supported shape is accepted; keep that
all-or-nothing behavior for later candidate renderers so unsupported
control-flow forms can still fall through cleanly.

The direct extern-call blocker is not part of Step 4's prepared branch/label
recovery contract. Treat it as the first Step 5 handoff reproving blocker:
repair only if it is required for the supervisor-selected x86 handoff proof,
and preserve the anti-overfit bar by using a semantic prepared-module call
route rather than a named fixture or canonical-asm shortcut.

The direct extern-call route now uses prepared call-plan wrapper/callee
identity, prepared call move bundles, prepared value homes, and prepared string
constant identity. Keep later call work on those semantic records; do not
replace it with fixture names or expected-assembly matching.

Global and pointer-backed selected-value compare-join returns remain
unsupported by this slice; a later packet should use the resolved same-module
global contract before enabling them.

The Step 4 review blocker in
`review/step4_compare_branch_slice_review.md` was previously repaired:
validation should not skip BIR blocks that cannot map to prepared control-flow
blocks, and the compare-branch renderer should not resolve true/false leaf
targets through raw label spelling. Preserve that bar while reworking the
current dirty route.

The compare-join return-move source check should stay anchored on the prepared
join return value home. The selected-value base home only explains how to
materialize the branch arm; the prepared out-of-SSA edge bundles and join
return bundle own the carrier-to-return authority.

Immediate compare-join return-context fixture mutations now explicitly refresh
the prepared `BeforeReturn` move bundle after changing prepared BIR topology.
Preserve that fixture metadata maintenance if later packets add more
post-prepare join carrier mutations.

Immediate-source compare-join edge moves now have explicit
`source_immediate_i32` value-location authority. Preserve the paired contract:
the prepared parallel-copy move carries the immediate source, and the matching
out-of-SSA move-resolution record must carry the same immediate source marker.

The rejected dirty route no longer remains in `module.cpp`. Do not reintroduce
missing-identity acceptance through compare-join-specific validator exceptions.
The producer-side identity blockers for retargeted condition targets, bridge
blocks, parallel-copy bundles, out-of-SSA move bundles, and renamed
compare-join carrier homes have been driven forward in the handoff fixture.
The branch-condition semantic authority negative test and the
`branch_join_adjust_then_xor` prepared return bundle blocker are now repaired.
The immediate selected-value compare-join edge move-bundle blocker is now
repaired, and `branch_join_immediate_then_xor` now has prepared return-bundle
authority. The remaining selected-proof failure is the unsupported same-module
global compare-join return context.

## Proof

Delegated proof command was run and preserved in `test_after.log`:
`backend_x86_prepared_handoff_label_authority` passed, and
`backend_x86_handoff_boundary` advanced past the previous `defined function
'branch_join_immediate_then_xor' has no prepared return move bundle` blocker.
It now fails later with `canonical prepared-module handoff rejected x86
value-location authority: defined function 'branch_join_global_then_xor' has an
unsupported global compare-join return context`. This is not acceptance proof;
it is the current Step 4 semantic blocker after immediate return-bundle
authority repair.
