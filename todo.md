Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add focused same-feature coverage

# Current Packet

## Just Finished

Step 4 added focused same-feature backend coverage in
`backend_aarch64_instruction_dispatch_test.cpp` for predecessor edge
publication of `base + sext(index) * 4`. The new test asserts that the
published pointer address keeps the pointer base in the target register,
materializes the scaled index through a distinct scratch register, and does
not reintroduce the stale `mov x9, #4` / `mul x10, x10, x9` clobber shape.

## Suggested Next

Supervisor should review and commit this Step 4 coverage slice, then advance
to broader validation and closure evidence.

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
- CTest registers the backend dispatch executable as
  `backend_aarch64_instruction_dispatch`, not
  `backend_aarch64_instruction_dispatch_test`; the delegated proof command did
  not execute the newly added backend coverage until the supervisor reran the
  corrected scope.

## Proof

Ran exactly:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00181_c)$'
```

Combined output is captured in `test_after.log`. The build completed after
recompiling and relinking `backend_aarch64_instruction_dispatch_test`; the
corrected CTest subset ran the focused backend dispatch test and
`c_testsuite_aarch64_backend_src_00181_c`.
