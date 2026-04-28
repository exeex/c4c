# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Reviewer route check `review/step4_accumulated_local_slot_renderer_route_review.md`
rejected the accumulated dirty Step 4 slice as drifting. The prepared
short-circuit and local i32 guard helper work remains directionally Step 4
only where it proves prepared branch, label, branch-plan, continuation,
parallel-copy, frame-slot, value-home, and return-ABI authority. The
standalone scalar local-slot return helper and the proposed i16 increment
route are not Step 4 control-flow work.

## Suggested Next

Split the dirty implementation before any further execution. The Step 4 packet
may keep only semantic prepared control-flow recovery: prepared short-circuit
and local i32 guard paths that are branch-owned and locally validate all
prepared control-flow authority before rendering. Retire or remove the
standalone scalar local-slot return additions from the Step 4 slice, or route
them through an explicit Step 3 scalar renderer packet with same-feature
positive and negative coverage.

Do not chase `minimal local-slot i16 increment guard route` as the next Step 4
blocker. Classify it as Step 3 scalar local-slot/width lowering unless the
supervisor deliberately opens a separate scalar recovery packet. If it is not
in that packet, record it as an explicit unsupported boundary for the current
Step 4 proof.

## Watchouts

Local-slot code is Step 4 only when it is inseparable from prepared
control-flow identity recovery. Any helper that runs before
`validate_prepared_control_flow_handoff` must own equivalent local checks for
prepared branch-plan, continuation-label, parallel-copy, value-home,
frame-slot, and return-ABI authority before rendering. Missing or drifted
prepared authority must reject the route; do not add raw-label fallback, named
fixture selection, or best-effort local-slot matching.

Standalone scalar local-slot return, mismatched load/store slot checks,
return-move/value-home drift checks, and scalar-width increment lowering need
Step 3-style same-feature positive and negative coverage. They are not
acceptance evidence for Step 4 unless tied to a prepared branch/control-flow
proof.

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
`minimal local-slot compare-against-immediate guard route` blocker and its
missing-prepared-branch-record negative. It now fails later with
`minimal local-slot i16 increment guard route: x86 prepared-module consumer
did not emit the canonical asm`. This is not acceptance proof; it is the
review trigger showing the dirty slice has crossed into scalar local-slot
renderer recovery and must be split, retired, or explicitly routed through
Step 3 before more execution.
