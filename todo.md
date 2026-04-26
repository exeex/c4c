# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics restored the bounded
prepared-module compare-against-zero branch renderer for the minimal scalar
branch lanes. The x86 prepared-module consumer now renders the branch predicate
and false label from authoritative prepared branch metadata, resolves true and
false leaf blocks through structured BIR label ids matching prepared label
identity, follows prepared value homes for register, stack, and rematerialized
entry values, requires prepared value homes and return bundles for parameter
leaf returns, and rejects missing prepared branch records, unmapped prepared
blocks, drifted prepared branch authority, and raw-label-only branch target
recovery.

## Suggested Next

Continue Step 4 with the next blocker exposed by the same proof: recover
prepared-module rendering for `scalar-control-flow compare-against-zero
compare-join lane with stack-backed parameter home`, where the x86
prepared-module consumer no longer follows the authoritative prepared stack
home through compare-join entry and return.

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
The newly recovered minimal compare-branch renderer intentionally stops before
join-transfer and parallel-copy rendering; the compare-join failure should stay
on the prepared join/out-of-SSA metadata path rather than widening the leaf
branch renderer into raw topology recovery.
The Step 4 review blocker in
`review/step4_compare_branch_slice_review.md` is repaired: validation no longer
skips BIR blocks that cannot map to prepared control-flow blocks, and the
compare-branch renderer no longer resolves true/false leaf targets through raw
label spelling.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and
`tests/backend/backend_x86_prepared_handoff_label_authority_test`, kept
`backend_x86_prepared_handoff_label_authority` passing, and advanced past the
minimal `scalar-control-flow compare-against-zero branch lane` and related
prepared-control-flow ownership checks after the prepared-label authority
review repair. The aggregate proof now fails later at `scalar-control-flow
compare-against-zero compare-join lane with stack-backed parameter home: x86
prepared-module consumer stopped following the authoritative prepared stack home
through compare-join entry and return`.
Canonical proof log: `test_after.log`.
