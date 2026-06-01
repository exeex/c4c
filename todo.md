Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Consume Prepared Call Plans And Boundary Move Facts

# Current Packet

## Just Finished

Completed `plan.md` Step 2 first narrow migration in `calls.cpp`.
`lower_before_call_moves` now threads each explicit
`PreparedCallBoundaryEffectPlan` into `lower_before_call_move`, and the
before-call `CallArgumentAbi` register-destination, register-source move branch
uses that effect as the authority gate while keeping `PreparedMoveBundle` and
`PreparedMoveResolution` provenance on the machine record. AArch64 register
conversion and instruction spelling remain local.

## Suggested Next

Continue Step 2 by migrating the next before-call `CallArgumentAbi`
register-destination subset in `calls.cpp` to consume the already-threaded
`PreparedCallBoundaryEffectPlan`, likely one stack-source or byval lane
register destination path, without widening into stack-slot destination copies.

## Watchouts

- The effect endpoint does not carry all AArch64 spelling details; the proven
  path still needs the move/binding/argument facts for register names,
  placements, occupied names, and f128 HFA width details.
- Do not require `classification_status == Available` for every register
  argument effect yet: existing f128 HFA lowering can validly proceed without
  an ABI binding while still using the effect's phase/destination/storage
  authority.
- Keep stack-slot destination copies and publication ordering out of the next
  packet unless the supervisor explicitly widens scope.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(call_boundary_effect_plan|prealloc_call_boundary_classification|aarch64_call_boundary_owner|codegen_route_aarch64_prepared_call_boundary_scalability)$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the exact proof output.
