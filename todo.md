# Current Packet

Status: Active
Source Idea Path: ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the HIR route and return to 831

## Just Finished

- Step 3 accepted the 832 aggregate-owner repair in `b556c6c9f`. The focused
  frontend HIR guard changed from the matching pre-repair 0/1 segfault to a
  post-repair 1/1 pass. The validated route is limited to LIR-owned aggregate
  function parameter/return type normalization and fail-closed owner checks.

## Suggested Next

- No further bounded executor packet belongs in 832. Plan-owner must close or
  switch this accepted ordered successor and return to 831 solely for its
  separately scoped 833 truthiness activation decision. Do not claim a full
  baseline pass or activate 831 from this progress record.

## Watchouts

- Do not add a null/default owner fallback, suppress the crash, weaken tests,
  or claim baseline clearance. 833 remains parked until this route returns
  accepted focused proof to 831.
- `typespec_aggregate_owner_key` is shared by layout and non-parameter paths;
  the repair remains at owned function type construction and does not alter
  that shared helper's behavior.

## Proof

- Evidence predecessor: `ba7958ee4` records the fresh exact 14/14 subset
  failure and the clean backend-enabled `f0fc85e4f^` HIR crash provenance.
  Step 1 must retain that provenance while narrowing the HIR owner seam.
- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_hir_tests$' > test_after.log`. The focused
  HIR subset passed 1/1; matching pre-repair `test_before.log` records the
  0/1 segfault and `test_after.log` records the accepted 1/1 pass.
