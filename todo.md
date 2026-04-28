# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 Recover Prepared Control-Flow Rendering Semantics repaired the prepared
short-circuit branch-condition target identity mismatch without restoring the
rejected local-slot short-circuit renderer. The x86 prepared control-flow
validator now accepts a branch-condition target mismatch only when the exact
prepared short-circuit branch plan owns that condition as a continuation
target with matching prepared true/false labels. Nearby negative coverage now
swaps the prepared RHS continuation true/false labels and proves that drifted
prepared branch-plan continuation targets are rejected before any unsupported
renderer path can accept them.

## Suggested Next

Continue Step 4 at the later semantic blocker now exposed by the selected
proof: the minimal local-slot short-circuit or-guard route advances past
prepared branch-condition target validation and is again rejected as an
unsupported x86 prepared-module shape. The next packet should either define a
semantic prepared renderer rule with nearby negative coverage or keep the
shape unsupported explicitly; do not revive the removed fixture-shaped
local-slot short-circuit renderer.

## Watchouts

Do not revive the removed local-slot short-circuit renderer as-is. Any future
short-circuit renderer needs a clearly defined semantic prepared lowering class
with nearby negative coverage for missing, drifted, and ambiguous prepared
identity. The current validator exception is intentionally narrow: it requires
the prepared short-circuit branch plan to own the continuation condition and
the exact prepared true/false continuation labels. The new same-feature
negative should remain before the unsupported positive route in the
short-circuit test order so it continues to prove the validator path while the
renderer shape is still unsupported.

The pointer-backed global compare-join route still validates the prepared
pointer-root global but does not complete semantic pointer dereference lowering;
the data emitter still publishes deferred initializer comments rather than
`.quad` symbol initializers. Keep that boundary explicit if the global
compare-join portion is accepted separately.

## Proof

Delegated proof command was run and preserved in `test_after.log`:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` advanced past the previous
`prepared branch condition targets drifted from prepared block targets`
blocker. It now fails later on the same route with `minimal local-slot
short-circuit or-guard route: x86 prepared-module consumer rejected the
prepared handoff with exception: x86 prepared-module consumer only supports a
minimal single-block i32 return terminator, a bounded equality-against-immediate
guard family with immediate return leaves including fixed-offset same-module
global i32 loads and pointer-backed same-module global roots, or one bounded
compare-against-zero branch family through the canonical prepared-module
handoff`. This is not acceptance proof; it is the current Step 4 semantic
blocker after the branch-condition target identity repair and the drifted
branch-plan continuation-target negative coverage.
