# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Recover Prepared Control-Flow Rendering Semantics

## Just Finished

Route review rejected the Step 4 helper-prefix renderer shortcut from
`review/step4_i8_local_byval_helper_prefix_slice_review.md`. The
`render_prepared_i8_local_byval_helper_function` path is not accepted as Step 4
progress: it is narrow i8/local-byval scalar helper growth, has no prepared
label, branch, join, or parallel-copy identity consumption, and must not be
used as the base for f32 or wider local-byval helper rendering.

The producer-side byval stack-offset publication insight is salvageable only as
a separate semantic authority slice: the producer can publish an authoritative
prepared stack copy destination for x86 byval pointer calls while preserving
the ABI pointer-register binding. That producer insight is not enough to accept
the current helper-prefix renderer, and it needs independent missing/drifted
stack-destination coverage before reuse.

## Suggested Next

Supervisor should split or reject the dirty implementation before further Step
4 execution: retire/remove the helper-prefix renderer shortcut and its
fixture-shaped acceptance proof, while preserving the producer byval
stack-offset publication only if it can stand as an independent producer
authority packet with positive plus missing/drifted stack-destination tests.

After that cleanup, the next executable Step 4 packet is the prepared
control-flow producer-publication route already named in `plan.md`: publish or
surface the missing two-segment loop-countdown branch-condition, join-transfer,
edge-transfer, and predecessor-owned parallel-copy bundle identities, or record
the missing identity as a semantic unsupported boundary. Do not continue with
f32 local-byval helper-prefix rendering in Step 4.

## Watchouts

Do not treat advancement to the adjacent f32 local-byval failure as proof of
semantic renderer recovery. Exact output fragments such as stack-byte stores
and helper-prefix instruction indexes are diagnostics only unless tied to a
general prepared statement/expression or control-flow identity contract.

Step 4 remains about prepared control-flow identity. Scalar local/byval helper
rendering belongs in Step 3 or a separate source idea unless it is inseparable
from prepared branch, label, branch-plan, continuation, or parallel-copy proof.

## Proof

Rejected proof from the reviewed dirty packet:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` advances past the previous i8
same-module local-byval helper-prefix rejection, including the prepared
destination stack copy and canonical `mov BYTE PTR [rsp], al`/`lea rdi, [rsp]`
fragments, then fails later at `bounded multi-defined helper same-module local
byval f32 call route: helper-prefix renderer still rejects helper function show
after addressed float-load support`. Reviewer judgment: route reset needed;
this is insufficient acceptance proof for Step 4 and should not drive the next
packet.
