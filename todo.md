Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate and prepare closure evidence

# Current Packet

## Just Finished

Step 5 ran the delegated broad AArch64 backend validation and captured closure
evidence in `test_after.log`. The validation passed, including
`c_testsuite_aarch64_backend_src_00181_c` and the focused
`backend_aarch64_instruction_dispatch` coverage.

## Suggested Next

Supervisor should review the completed repair, coverage, and validation
evidence, then decide whether to close the active lifecycle state.

## Watchouts

- Do not special-case `00181`, Tower of Hanoi symbols, or one exact global
  array shape.
- Do not weaken or disable c_testsuite expectations.
- Do not restore broad target-local fallback selection as a substitute for
  complete prepared facts.
- Keep unrelated AArch64 calls, dispatch, and c_testsuite cleanup outside this
  route.
- The direct global-array references in `PrintAll` and `main` use `adrp/add`
  forms and the binary contains local `.data` symbols for `A`, `B`, and `C`;
  avoid starting with global storage unless Step 2 contradicts this evidence.
- In `Move`, generated lines around the first failing pattern are
  `build/c_testsuite_aarch64_backend/src/00181.c.s` lines 429-435 for
  `source[i]` and 477-483 for `dest[j]`.
- The evidence points at register clobbering during nested value
  materialization. A repair that only restores broad target-local fallback
  source reconstruction would be route drift.
- Keep the fix semantic: the same hazard can occur whenever an outer add/sub
  keeps a pointer base in the target register while a nested RHS expression
  uses that same register as its scratch.
- Ownership expanded beyond the original delegated code file because the exact
  stale producer was `dispatch_edge_copies.cpp` edge/select-chain publication.
  The expansion is still within the same AArch64 value-publication failure
  family and did not touch expectations or unrelated lowering routes.
- Route cause analysis: nested AArch64 value/edge publication clobbered the
  pointer base during scaled-index materialization. The repair was semantic
  register-hazard handling for nested publication, not a named testcase
  shortcut for `00181`.

## Proof

Ran exactly:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|c_testsuite_aarch64_backend_)'
```

Combined output is captured in `test_after.log`. The build completed and the
CTest subset passed: 248 tests passed, 0 failed. The run confirms
`c_testsuite_aarch64_backend_src_00181_c` passed and also includes
`backend_aarch64_instruction_dispatch`.
