# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Reprove X86 Handoff And Decide Lifecycle Outcome

## Just Finished

Step 5 Reprove X86 Handoff And Decide Lifecycle Outcome recovered the bounded
direct extern-call prepared-module route in the x86 prepared-module consumer.
The renderer now accepts a single-block direct extern call sequence through
semantic prepared call plans and prepared value-location bundles, uses
authoritative `BeforeCall` metadata for argument ABI registers, authoritative
`AfterCall` metadata plus prepared value homes for call-result captures, emits
the x86-64 call-alignment adjustment, and renders referenced prepared string
constants for the accepted route. The route rejects missing argument/result
call-bundle authority instead of reopening local ABI fallback.

## Suggested Next

Continue Step 5 with the next x86 handoff blocker exposed by the same proof:
`scalar-control-flow compare-against-zero joined branch lane: x86
prepared-module consumer rejected the prepared handoff with exception:
canonical prepared-module handoff rejected x86 control-flow label authority:
compare-join join block has no authoritative prepared block id`.

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
adjacent rematerialized compare-join lane, the compare-join authority
rejection/relocation checks after the compare-join review repair, and the
bounded direct extern-call prepared-module route plus its prepared call-bundle
drift checks. The aggregate proof still fails later at `scalar-control-flow
compare-against-zero joined branch lane: x86 prepared-module consumer rejected
the prepared handoff with exception: canonical prepared-module handoff rejected
x86 control-flow label authority: compare-join join block has no authoritative
prepared block id`.
Canonical proof log: `test_after.log`.
