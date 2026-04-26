# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics restored the bounded
prepared-module compare-against-zero compare-join renderer for the
stack-backed and rematerialized parameter-home lanes. The x86 prepared-module
consumer now enters that route through the authoritative prepared
materialized compare-join contract, validates the published out-of-SSA
parallel-copy bundles and move bundles for both join edges, renders each
return arm from the prepared parameter home and return bundle, and rejects
missing join-transfer or parallel-copy authority instead of reopening raw
compare-join recovery. The follow-up review repair removed mutable
prepared-name-table emission, rejects global and pointer-backed compare-join
return contexts before publishing assembly, and lets non-matching one-param
join-bearing candidates fall through unless this compare-join route owns the
prepared branch contract.

## Suggested Next

Continue with the next blocker exposed by the same proof:
`bounded direct extern-call prepared-module route: x86 prepared-module
consumer did not emit the canonical asm`. The compare-join slice is past the
requested stack-backed lane, adjacent rematerialized lane, missing
join-transfer rejection, missing parallel-copy rejection, predecessor-owned
bundle placement, successor-entry relocation, and multi-parameter
compare-driven rejection checks.

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
The compare-join renderer intentionally remains bounded to the prepared
param-zero materialized compare-join contract. It should continue to use
prepared branch labels, join-transfer metadata, published parallel-copy
bundles, out-of-SSA move bundles, prepared value homes, and prepared return
bundles as authority rather than recovering from raw labels or fixture shape.
Global and pointer-backed selected-value compare-join returns remain
unsupported by this slice; a later packet should use the resolved same-module
global contract before enabling them.
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
minimal compare-branch lanes, the requested `scalar-control-flow
compare-against-zero compare-join lane with stack-backed parameter home`, the
adjacent rematerialized compare-join lane, and the compare-join authority
rejection/relocation checks after the compare-join review repair. The aggregate
proof still fails later at `bounded direct extern-call prepared-module route:
x86 prepared-module consumer did not emit the canonical asm`.
Canonical proof log: `test_after.log`.
