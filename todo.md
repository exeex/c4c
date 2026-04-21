# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Prepared-Module Or Call-Bundle Seam
Plan Review Counter: 1 / 4
# Current Packet

## Just Finished

Plan Step 2.1 repaired the next idea-61 same-module helper composition seam in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`. The x86 helper
direct-extern renderer now accepts prepared stack-passed late pointer-class
arguments generically alongside the already-supported stack `f128` lane, and
the pointer-source address materialization keeps the caller frame biased
correctly after reserving outgoing stack argument space.

I added focused boundary coverage in
`tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp` for a
same-module helper-prefix route with three tiny byval pointer lanes interleaved
with four promoted float lanes so the final pointer argument lands on the
prepared stack lane. That test now passes and proves the widened acceptance
path: x86 emits the helper body, reserves stack argument space, materializes
the late pointer into the outgoing stack slot, and still returns through a
trivial `main`.

The reduced reproducers `/tmp/fa4b_helper_only.c` and `/tmp/fa4d_helper_only.c`
now emit x86 asm successfully after this patch. The full
`c_testsuite_x86_backend_src_00204_c` route moves past the old top-level
minimal-return rejection and now fails later with a different diagnostic:
`x86 backend emitter requires the authoritative prepared short-circuit handoff
through the canonical prepared-module handoff`.

## Suggested Next

Take the next packet on the new full-`00204` blocker now that the late
stack-passed pointer helper lane is accepted. The next seam is the surviving
authoritative prepared short-circuit handoff requirement that appears after the
repaired helper-prefix route, not another revisit of the old top-level
minimal-return rejection.

## Watchouts

- The widened helper path now depends on stack-byte bias when a late pointer
  argument is written into the outgoing variadic stack area. Keep the new
  helper-prefix boundary guard green if the next packet touches prepared stack
  address rendering again.
- `/tmp/fa4b_helper_only.c` and `/tmp/fa4d_helper_only.c` are now useful
  sentinels for this seam: they used to fail on the old top-level rejection and
  now emit x86 asm with the late pointer written into the outgoing stack lane.
- The full `00204` family is still red, but the failing diagnostic changed to
  the authoritative prepared short-circuit handoff requirement. The next packet
  should start from that later blocker instead of re-debugging the helper
  variadic stack pointer route.
- `backend_x86_handoff_boundary` stays green after the helper direct-extern
  widening, so there is no supported-path regression inside the touched backend
  files from this packet.

## Proof

Ran the delegated proof command:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'; } > test_after.log 2>&1`.
Current proof is mixed in the final workspace state:
`backend_x86_handoff_boundary` passes, while
`c_testsuite_x86_backend_src_00204_c` still fails. The old top-level
minimal-return rejection is gone; the new failing diagnostic is:
`x86 backend emitter requires the authoritative prepared short-circuit handoff
through the canonical prepared-module handoff`.
The canonical proof log remains `test_after.log`. Additional reducer proof for
this packet:

- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu /tmp/fa4b_helper_only.c`
  now succeeds and emits the late pointer argument into an outgoing stack slot
  before `printf`.
- `build/c4cll --codegen asm --target x86_64-unknown-linux-gnu /tmp/fa4d_helper_only.c`
  now succeeds and emits the same late pointer stack lane alongside the already
  supported stack `f128` arguments.

Together, those results show this packet repaired one real idea-61 helper
composition seam and moved full `00204` to a later blocker.
