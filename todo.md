Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consume Prepared Call Plans And Boundary Move Facts

# Current Packet

## Just Finished

Completed another narrow `plan.md` Step 2 migration in `calls.cpp`. The
before-call `CallArgumentAbi` register-destination path for local-frame-address
materialization and register byval lane sources now uses the already-threaded
`PreparedCallBoundaryEffectPlan` explicit-move effect as its phase,
destination, storage, and order authority. `PreparedMoveBundle` and
`PreparedMoveResolution` remain available on the machine record for provenance,
while AArch64 register/address conversion and instruction spelling stay local.

## Suggested Next

Continue Step 2 by migrating the next before-call `CallArgumentAbi`
register-destination stack-source subset in `calls.cpp`, likely the scalar FPR
or binary128 frame-slot-to-register path, to consume the
`PreparedCallBoundaryEffectPlan` authority without widening into stack-slot
destination copies.

## Watchouts

- The effect endpoint still does not carry all AArch64 spelling details; the
  proven paths still need move/binding/argument facts for register names,
  placements, occupied names, selected source memory, and f128 HFA details.
- The newly migrated local-frame-address/register-byval path uses the effect
  destination bank and no-binding contiguous width; keep any further width
  authority changes local to one branch at a time.
- Do not require `classification_status == Available` for every register
  argument effect yet: existing f128 HFA lowering can validly proceed without
  an ABI binding while still using the effect's phase/destination/storage
  authority.
- Keep stack-slot destination copies and publication ordering out of the next
  packet unless the supervisor explicitly widens scope.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(call_boundary_effect_plan|prealloc_call_boundary_classification|aarch64_call_boundary_owner|codegen_route_aarch64_prepared_call_boundary_scalability)$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the exact proof output.
