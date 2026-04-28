# Current Packet

Status: Active
Source Idea Path: ideas/open/121_x86_prepared_module_renderer_recovery.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Reset The Accumulated Scalar Local-Slot Route

## Just Finished

Reviewer report
`review/step3_accumulated_scalar_local_slot_route_review.md` rejected the
accumulated Step 3 scalar local-slot route as drifting. The dirty implementation
is too broad for one Step 3 packet, still mixes Step 4 short-circuit/control-flow
authority work with several scalar local-slot helpers, and the scalar strategy
is becoming exact-sequence-shaped around instruction counts and instruction
indexes.

The current proof is red at `minimal local-slot add-chain guard route`, so the
dirty slice is not acceptance-ready and should not be extended with another
case-specific add-chain helper.

## Suggested Next

Next executor packet must split or retire the dirty implementation before any
new scalar renderer support:

- keep Step 4 short-circuit/control-flow authority changes separate from Step 3
  scalar local-slot work;
- keep any already-valid i16/i64 subtract-return scalar work separate from the
  red add-chain/guard-lane problem;
- remove, park, or rewrite scalar helpers selected mainly by exact instruction
  counts, fixed instruction indexes, or fixture topology;
- do not add a new helper just to make `minimal local-slot add-chain guard
  route` pass.

If scalar work continues after that split, the next Step 3 implementation must
start from a generalized prepared local-slot expression/statement renderer. It
must consume prepared frame-slot ids, prepared memory accesses, prepared value
homes, typed scalar operations, return moves, and branch conditions as semantic
records. It must cover nearby add/sub guard-lane and return cases through the
same route, with negative coverage for missing/drifted memory access,
frame-slot id, access size, load/store authority, value homes, return
source/destination, and branch-condition identity.

## Watchouts

Do not commit the accumulated dirty slice as-is. Do not treat a green
add-chain positive alone as enough proof. If the remaining scalar route cannot
be expressed without exact instruction-sequence dispatch, record the boundary as
unsupported for this packet and stop for plan review.

Step 4 work belongs in Step 4 unless it is inseparable from the delegated scalar
proof. No-branch local-slot return, scalar-width increment, subtract, add-chain,
and guard-lane expression rendering belong in Step 3 only if they share a
general prepared local-slot expression/statement route.

## Proof

Current `test_after.log` is red and is not acceptance proof. The command run was:
`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`.

Configure and build passed. `backend_x86_prepared_handoff_label_authority`
passed. `backend_x86_handoff_boundary` failed with
`minimal local-slot add-chain guard route: x86 prepared-module consumer did not
emit the canonical asm`.
