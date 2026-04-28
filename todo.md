# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Step 4 repaired the bounded multi-defined-function same-module symbol-call
prepared-module route. The x86 prepared-module consumer now renders the
same-module indirect symbol calls to the defined callee, consumes the prepared
AfterCall call-result bundles, consumes prepared BeforeCall printf argument
bundles, emits local-slot stores and loads from the prepared frame-slot memory
operands, follows prepared load-result register homes for later argument moves,
emits the route's string data, and keeps the existing call-bundle drift and
removal negatives on prepared authority. The route-shaped repeated
`.intel_syntax/.text` switch was replaced with a general per-function text
header policy.

The delegated proof now advances past the same-module symbol-call positive, its
prepared BeforeCall/AfterCall authority negatives, the prepared frame-access
drift mutation, and the prepared load-home drift mutation. Acceptance is still
blocked later in the same `backend_x86_handoff_boundary` binary by the separate
`bounded multi-defined helper same-module local byval call route`, where the
public helper-prefix renderer still rejects helper function `show` after
producer-side symbol-access publication.

## Suggested Next

Next packet should address the newly exposed same-module local-byval helper
prefix renderer boundary, starting at
`render_prepared_bounded_same_module_helper_prefix_if_supported` and keeping it
separate from the completed symbol-call renderer slice.

## Watchouts

The symbol-call repair is intentionally bounded to single-block i32 local-slot
call lanes with same-module symbol callees and direct extern printf calls. It
validates prepared call plans, prepared indirect callee identity, prepared
BeforeCall/AfterCall bundles, prepared memory-access records, and prepared load
homes; it should not be widened into byval, sret, f32/f128 helper, or generic
aggregate helper rendering without a separate packet.

## Proof

Ran the delegated proof command for this packet:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` now advances past the delegated
same-module symbol-call route, including focused prepared frame-access drift and
prepared load-home drift mutation checks, then fails later at `bounded
multi-defined helper same-module local byval call route: helper-prefix renderer
still rejects helper function show after producer-side symbol access
publication`. `test_after.log` contains the red delegated proof output.
