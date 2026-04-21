# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 2.1
Current Step Title: Repair The Selected Scalar Prepared/Emitter Seam
Plan Review Counter: 2 / 4
# Current Packet

## Just Finished

Step 2.1 inspected the shared prepared pointer-value / addressing seam behind
the reduced `match` route and stopped at a real packet-ownership blocker. The
owned prepared structs in `prealloc.hpp` and the x86 local-slot consumers
already support `PreparedAddressBaseKind::PointerValue` with
`pointer_value_name` plus `can_use_base_plus_offset`, but the reduced
`c_testsuite_x86_backend_src_00204_c` route still reaches x86 without the
pointer-address metadata needed to describe the `%t22` / `%t24` / `%t34`
stores generically. The producer for that metadata is
`src/backend/prealloc/stack_layout.cpp`, which is outside this packet's owned
files, so this slice stopped without a shippable code repair.

## Suggested Next

Send the next idea-60 packet with ownership over
`src/backend/prealloc/stack_layout.cpp` plus the adjacent prepared/x86
consumers, then repair how the reduced `match` route publishes pointer-indirect
prepared addressing for the `%t22` / `%t24` / `%t34` family. Keep proof on the
same backend handoff subset and only revisit x86-side changes if the producer
starts emitting a durable `PointerValue` address that the existing consumers
still fail to follow.

## Watchouts

- Do not reopen the rejected x86-local pointer-binary experiment. The owned x86
  consumer code already has `PointerValue` address handling, so another
  emitter-local matcher would hide the real producer gap.
- The critical ownership fact is now concrete: `PreparedAddress` publication
  for pointer-indirect accesses is built in `src/backend/prealloc/stack_layout.cpp`,
  not in the owned `prealloc.hpp` declarations.
- Keep the blocker in idea 60 unless the producer-side inspection shows the
  route actually falls back before scalar emission into a different leaf.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the idea-60 scalar
restriction. Supporting investigation confirmed that the owned x86 consumers
already resolve `PreparedAddressBaseKind::PointerValue`, while
`src/backend/prealloc/stack_layout.cpp` is the producer that builds
pointer-indirect `PreparedMemoryAccess` records. The reduced `match` route
still reaches x86 without enough published pointer-address metadata for the
`%t22` / `%t24` / `%t34` store family, so this packet is blocked outside its
owned files. Canonical proof log: `test_after.log`.
