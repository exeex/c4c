Status: Active
Source Idea Path: ideas/open/561_prepared_local_address_base_plus_offset_boundary_evidence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Only The Named Boundary

# Current Packet

## Just Finished

Step 4 repaired the RV64-owned prepared local-memory consumer boundary for
pointer-value base-plus-offset accesses whose pointer base has a coherent
prepared stack-slot home.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.hpp`
- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Implementation summary:

- Added `prepared_pointer_value_stack_home_base_offset()` to validate a
  pointer-value BPO access whose pointer base is a stack-slot home against the
  prepared value-home table, frame slot, stack object, stack-frame bounds, and
  local-memory access contract.
- Kept specialized byval/sret stack homes out of the generic stack-home helper
  so their existing stricter fail-closed contracts remain authoritative.
- Taught the prepared local-memory object-fragment store/load paths to
  materialize a valid stack-homed pointer base into a scratch GPR before using
  the existing pointer-value base-plus-offset load/store emitters.
- Updated unsupported-local-memory diagnostics to recognize the same coherent
  stack-home pointer-base fact accepted by emission.
- Added the tracked focused RV64 object-emission test
  `builds_prepared_pointer_value_scalar_stack_home_local_object()`, using a
  normal prepared local pointer-value stack home independent of
  `src/20000519-1.c`. The test asserts the emitted object loads `%p` from its
  stack home before the existing `sh`/`lh` pointer-value BPO sequence.

## Suggested Next

Execute Step 5 by rerunning the Step 1 representative RV64 command and the
supervisor-selected local-memory proof subset. If the representative now
advances to a different first bad fact, record that owner without widening this
slice.

## Watchouts

- Keep the repair semantic: do not special-case `src/20000519-1.c`; the route
  should handle prepared pointer-value base-plus-offset accesses whose pointer
  base has a coherent stack-slot home.
- Do not reconstruct local-memory facts from RV64 target-specific instruction
  shapes.
- Do not combine this route with direct-call metadata repair.
- Do not weaken unsupported accounting, expected output, tests, or prepared
  admission contracts.
- Do not use named-case shortcuts for retained torture representatives.
- Keep `review/557_step13_vector_local_memory_review.md` untouched unless the
  supervisor explicitly brings it into scope.
- The existing sret stack-home pointer-value object test is not enough by
  itself for this route because it depends on sret-home publication and does not
  exercise a normal prepared local pointer-value stack home.
- The temporary Step 3 probe intentionally failed before repair and was removed
  from tracked tests; the equivalent focused stack-home pointer-value local
  object test is now tracked and passing.
- The generic stack-home pointer-base path intentionally excludes byval and
  sret homes; those lanes still use their existing specialized helpers and
  fail-closed shape tests.

## Proof

Step 4 validation:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log && git diff --check -- todo.md src/backend/mir/riscv/codegen tests/backend/mir/backend_riscv_object_emission_test.cpp
```

Result: passed. `cmake --build --preset default` completed,
`ctest --test-dir build -j --output-on-failure -R '^backend_'` completed with
all 345 backend tests passing, and `git diff --check` passed for the delegated
paths. Supervisor regression comparison also passed with:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

The non-decreasing mode was used because the new focused assertion lives inside
the existing `backend_riscv_object_emission` CTest binary, so the CTest test
count remains 345. Proof log: `test_after.log`.
