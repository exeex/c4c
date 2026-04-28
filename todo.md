# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics is in rework after
`review/step4_current_x86_control_flow_route_review.md`. The current dirty
x86 control-flow slice is rejected as route drift: it weakens validation by
accepting missing prepared identity through compare-join-specific skips and a
bridge-block path that relies on raw BIR label topology. Do not treat the
dirty implementation or its recorded failing proof as accepted progress.

## Suggested Next

Rework Step 4 before adding new capability. The next executor packet should
replace the rejected validator escape paths with semantic prepared identity
publication or direct prepared identity consumption:

- remove or narrow any skip for a prepared control-flow block that has no live
  BIR block identity unless the exact skipped label is proven to be owned by
  the authoritative compare-join branch/edge metadata
- replace the unprepared bridge-block raw-label topology proof with prepared
  identity publication, an explicit prepared bridge-block record, or stop with
  a blocker/split request if bridge blocks are now a real compiler concept
- require compare-join parallel-copy authority to prove the authoritative edge
  predecessor relationship, or add same-feature negative coverage before using
  a fallback as progress
- keep branch-condition stale-record handling from silencing live prepared
  control-flow drift; add stale-vs-live negative coverage if that distinction
  remains necessary

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

The rejected dirty route specifically added or depended on: broad
prepared-block skip ownership that does not tie the skipped label to the
authoritative edge; an unprepared bridge-block exception proven by raw label
strings; a parallel-copy fallback that does not prove the edge predecessor
relationship; and branch-condition metadata skips without stale-vs-live
coverage. Rework those before following the next failing x86 handoff message.

If semantic prepared identity publication/consumption cannot be expressed with
the current prepared-module records, stop and report the blocker or request a
plan split. Do not continue Step 4 by adding another validator exception for
the next failing x86 handoff message.

## Proof

No acceptance proof for the current dirty Step 4 route. The reviewed
`test_after.log` command rebuilt the two x86 targets, kept
`backend_x86_prepared_handoff_label_authority` passing, and still failed
`backend_x86_handoff_boundary` at `canonical prepared-module handoff rejected
x86 control-flow label authority: compare-join edge has no authoritative
prepared parallel-copy bundle`; the reviewer found the route itself drifting,
so that proof is rework evidence only.
