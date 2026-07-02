Status: Active
Source Idea Path: ideas/open/517_residual_scalar_f32_f64_cast_object_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate Representatives And Backend Subset

# Current Packet

## Just Finished

Step 5 - Validate Representatives And Backend Subset completed for idea 517.

Fresh representative proof:

- `tests/c/external/gcc_torture/src/cvt-1.c`: `CVT1_EXIT=2 OBJECT=no`.
  The row advances past the old `unsupported_floating_cast` gate and now
  reports
  `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
- `tests/c/external/gcc_torture/src/pr66233.c`: `PR66233_EXIT=2 OBJECT=no`.
  The row also advances past the old `unsupported_floating_cast` gate and now
  reports the same separate ambiguous non-parallel multi-source
  stack-destination boundary.
- `tests/c/external/gcc_torture/src/920618-1.c`:
  `CASE920618_EXIT=2 OBJECT=no`. This remains intentionally split/fail-closed
  at `unsupported_floating_cast` pending explicit FP-immediate materialization
  authority.

Backend validation:

- `cmake --build --preset default` completed with no build work needed.
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` passed:
  345/345 backend tests.
- `git diff --check -- todo.md` passed.

Closure handoff:

- The accepted scalar F32/F64 FP-to-I32/I64 register-home boundary is
  implemented and covered.
- The old `unsupported_floating_cast` boundary is advanced for `cvt-1.c` and
  `pr66233.c`; their remaining failure is a different move-bundle authority
  boundary and should not block closure of this scalar-cast idea.
- `920618-1.c` is correctly quarantined until a producer/prepared
  FP-immediate authority exists.
- Active idea 517 appears ready for supervisor/plan-owner closure review after
  this validation, with residual work split outside this route.

## Suggested Next

Return to the supervisor for closure review. Do not start residual
multi-source stack-destination or FP-immediate materialization work from this
plan.

## Watchouts

Residual risks are bounded and out of route: ambiguous non-parallel
multi-source stack-destination authority remains for the two accepted
representatives after the scalar-cast gate, and typed FP-immediate
materialization remains absent for `src/920618-1.c`. Keep F128/helper ABI,
local-memory, aggregate/byval, stack-frame, branch/select, call/return, and
`conversion.c` work out of this route.

## Proof

Proof is captured in `test_after.log`:

- `cmake --build --preset default`
- focused `c4cll --codegen obj` probes for `src/cvt-1.c`, `src/pr66233.c`, and
  `src/920618-1.c`
- `ctest --test-dir build -j --output-on-failure -R '^backend_'`
- `git diff --check -- todo.md`
