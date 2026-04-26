# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics repaired the
joined-branch compare-join return-move source mismatch in the x86
prepared-module renderer. The compare-join return arm now validates the
prepared return bundle against the prepared home of the join block's actual
returned value and verifies the render context still belongs to the prepared
join block, while selected parameter materialization remains driven by
prepared value homes rather than raw labels or fixture names.

## Suggested Next

Continue Step 4 on the next joined-branch handoff blocker exposed by the same
proof:
`canonical prepared-module handoff rejected x86 control-flow label authority:
BIR block 'carrier.zero' has no prepared control-flow block`.

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
For joined-branch compare-join ownership cases, the BIR block index may need to
come from the prepared compare-join context's owned block pointer when raw BIR
label ids are intentionally drifted; keep any later value-location repair on
prepared move/home authority rather than reintroducing raw label recovery.
Global and pointer-backed selected-value compare-join returns remain
unsupported by this slice; a later packet should use the resolved same-module
global contract before enabling them.
The Step 4 review blocker in
`review/step4_compare_branch_slice_review.md` is repaired: validation no longer
skips BIR blocks that cannot map to prepared control-flow blocks, and the
compare-branch renderer no longer resolves true/false leaf targets through raw
label spelling.
The compare-join return-move source check should stay anchored on the prepared
join return value home. The selected-value base home only explains how to
materialize the branch arm; the prepared out-of-SSA edge bundles and join
return bundle own the carrier-to-return authority.

## Proof

`( cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$' ) > test_after.log 2>&1`
rebuilt `tests/backend/backend_x86_handoff_boundary_test` and
`tests/backend/backend_x86_prepared_handoff_label_authority_test`, kept
`backend_x86_prepared_handoff_label_authority` passing, and advanced past the
requested `defined function 'branch_join_adjust' compare-join return move
source drifted from selected parameter home` blocker. The aggregate proof still
fails later after aborting at `canonical prepared-module handoff rejected x86
control-flow label authority: BIR block 'carrier.zero' has no prepared
control-flow block`.
Canonical proof log: `test_after.log`.
