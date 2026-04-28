# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics was reviewed in
`review/step4_global_short_circuit_slice_review.md`. That review rejected the
current whole dirty global/short-circuit slice as drifting and recommended a
plan/todo rewrite before more execution.

Do not treat the accumulated dirty route as accepted Step 4 progress. The
review says the same-module global compare-join work and the short-circuit
execution-site clarification may be separable semantic pieces, but the
local-slot short-circuit renderer is too fixture-shaped to accept as-is.

## Suggested Next

Continue Step 4 only on one bounded route chosen by the supervisor:

1. Split and validate only the semantic same-module global compare-join pieces
   plus the short-circuit execution-site clarification, leaving the
   local-slot short-circuit renderer out of the commit.
2. Rework the short-circuit renderer around a clearly defined semantic
   prepared lowering class, with nearby positive and negative coverage for
   missing, drifted, and ambiguous prepared identities before claiming
   progress.
3. Repair the missing bridge identity first: the selected proof fails on
   `contract.rhs.bridge` having no prepared block id, so the next packet may
   need to publish or preserve exact prepared identity for that bridge before
   any x86-side renderer acceptance path is valid.

Whichever route is chosen, do not expand or commit the current local-slot
short-circuit renderer as-is, do not add raw label recovery, and do not add
x86-side fallback acceptance for missing prepared identity.

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

Direct and pointer-backed same-module global selected-value compare-join
returns now use the resolved prepared same-module global contract. Pointer-root
data emission still uses the existing x86 data emitter behavior for symbol
initializer globals: it emits deferred initializer comments rather than `.quad`
symbol initializers.

The Step 4 review blocker in
`review/step4_compare_branch_slice_review.md` was previously repaired:
validation should not skip BIR blocks that cannot map to prepared control-flow
blocks, and the compare-branch renderer should not resolve true/false leaf
targets through raw label spelling. Preserve that bar while reworking the
current dirty route. The newer
`review/step4_global_short_circuit_slice_review.md` is the controlling review
for the active dirty slice: it rejects the whole slice as commit-ready and
requires either splitting out only the semantic portions, reworking the
short-circuit lowering class with negative coverage, or repairing missing
bridge identity first. Short-circuit branch-condition target validation has a
narrow exception only when exact prepared short-circuit branch-plan
continuation ownership is present; add negative coverage before using that
exception as a foundation for broader short-circuit rendering.

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

Do not carry forward the previous todo wording that described the minimal
local-slot short-circuit branch-target, unsupported-renderer, and
execution-site assertion blockers as repaired or clarified. The review did not
accept that route as a coherent Step 4 packet. If the supervisor chooses to
split out the global and execution-site portions, keep the pointer-backed
global behavior explicit as a remaining boundary: the reviewed dirty route
validates the pointer-root global but does not yet prove complete semantic
pointer dereference lowering.

The current selected-proof failure remains useful as a blocker, not acceptance
proof: `contract.rhs.bridge` lacks a prepared block id. Prefer repairing
producer-side identity for that bridge over adding consumer-side exceptions.

## Proof

Delegated proof command was run and preserved in `test_after.log`:
`backend_x86_prepared_handoff_label_authority` passed, and
`backend_x86_handoff_boundary` advanced past the previous
`prepare no longer classifies the branch-owned short-circuit handoff as
critical-edge executable` assertion. It now fails later with `canonical
prepared-module handoff rejected x86 control-flow label authority: BIR block
'contract.rhs.bridge' has no prepared block id`.

This is not acceptance proof. After
`review/step4_global_short_circuit_slice_review.md`, treat it only as blocker
evidence for the next Step 4 route. Fresh validation is required for any split
semantic subset or reworked short-circuit renderer.
