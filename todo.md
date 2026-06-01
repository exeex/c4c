Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consume Prepared Call Plans And Boundary Move Facts

# Current Packet

## Just Finished

Completed another narrow `plan.md` Step 2 migration in `calls.cpp`. The
before-call `CallArgumentAbi` prior-preserved register-destination path now
uses the already-threaded `PreparedCallBoundaryEffectPlan` explicit-move
effect as its phase, destination, storage, and order authority. The
`PreparedMoveBundle`, `PreparedMoveResolution`, binding, and argument facts
still remain on the machine record or local lowering path for provenance,
source selection, and AArch64 spelling.

## Suggested Next

Continue Step 2 with a supervisor-selected review or the next remaining
prepared call-boundary publication path; keep stack-slot destination copies and
publication ordering out of scope unless explicitly delegated.

## Watchouts

- The effect endpoint still does not carry all AArch64 spelling details; the
  proven paths still need move/binding/argument facts for register names,
  placements, occupied names, selected source memory, scalar FP views, and f128
  HFA details.
- The newly migrated scalar FPR frame-slot path uses the effect destination
  bank and contiguous width while retaining binding data for destination name,
  placement, occupied names, and scalar FP view.
- The newly migrated binary128 frame-slot path uses the effect destination bank
  and contiguous width while retaining binding data for destination name,
  placement, occupied names, and the local q-register view.
- The newly migrated GPR frame-slot path uses the effect destination bank and
  contiguous width while retaining binding/move data for destination name,
  placement, and instruction spelling; binding occupied names remain preferred
  when available.
- The newly migrated prior-preservation path now gates on the selected explicit
  move effect and uses the effect destination endpoint for bank, width, and
  occupied-register authority while retaining binding/move/argument facts for
  concrete register spelling and preserved source selection.
- Do not require `classification_status == Available` for every register
  argument effect yet: existing f128 HFA lowering can validly proceed without
  an ABI binding while still using the effect's phase/destination/storage
  authority.
- Keep stack-slot destination copies and publication ordering out of the next
  packet unless the supervisor explicitly widens scope.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(call_boundary_effect_plan|prealloc_call_boundary_classification|aarch64_call_boundary_owner|codegen_route_aarch64_prepared_call_boundary_scalability)$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the exact proof output.
