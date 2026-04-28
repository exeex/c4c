# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics reworked the rejected
dirty x86 control-flow slice back to the least misleading state: the broad
prepared-block skip, raw-label bridge-block acceptance path, same-successor
parallel-copy fallback, and stale-looking branch-condition skip were removed
from `src/backend/mir/x86/module/module.cpp`. The remaining code consumes the
direct prepared edge transfer and published parallel-copy bundle only, so the
slice now stops on missing prepared identity instead of accepting it.

## Suggested Next

Lifecycle decision: keep the blocker inside the active idea 121 plan and
continue Step 4 as a producer identity publication repair before returning to
the x86 consumer. The current prepared records do not publish enough identity
for the x86 consumer to accept the handoff without weakening validation: the
proof now fails on a prepared control-flow block id that no longer maps to a
live BIR block after the handoff test mutates the carrier labels.

The next executor packet should repair prepared producer publication for the
mutated or bridge carrier blocks and their authoritative parallel-copy bundles,
then re-run the x86 consumer proof with strict missing/drifted identity
rejection. Do not add another x86-side validator escape as the repair.

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

The rejected dirty route no longer remains in `module.cpp`. Do not reintroduce
missing-identity acceptance through compare-join-specific validator exceptions:
the current blocker is a prepared-record publication gap, not an x86 rendering
shape gap. The bridge/passthrough cases visible in the tests need explicit
prepared identity or a documented producer-side concept before x86 should
consume them.

## Proof

Delegated proof command was run and preserved in `test_after.log`:
`backend_x86_prepared_handoff_label_authority` passed, and
`backend_x86_handoff_boundary` failed with `canonical prepared-module handoff
rejected x86 control-flow label authority: prepared block #2 does not match
any BIR block by label id`. This is blocker evidence after removing the
rejected validator escapes, not acceptance proof.
