Status: Active
Source Idea Path: ideas/open/18_aarch64_cts_00181_runtime_regression_reopen.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce and classify the current failure

# Current Packet

## Just Finished

Step 1 reproduced and classified the current
`c_testsuite_aarch64_backend_src_00181_c` runtime failure without
implementation or expectation edits. The delegated proof rebuilt the current
tree and reran the exact targeted CTest subset, which still fails as
`[RUNTIME_NONZERO]` with `exit=Segmentation fault` and no captured
stdout/stderr in `test_after.log`.

Inspected evidence:

- Source: `tests/c/external/c-testsuite/src/00181.c` uses global arrays
  `A`, `B`, and `C`, then passes their addresses into recursive `Hanoi` and
  `Move(int *source, int *dest)`.
- Generated assembly artifact:
  `build/c_testsuite_aarch64_backend/src/00181.c.s`.
- Linked binary artifact:
  `build/c_testsuite_aarch64_backend/src/00181.c.bin`.
- The assembly emits global storage for `A`, `B`, and `C` in `.data`, and the
  linked symbol table places local `A`, `B`, and `C` symbols in `.data`.
- The strongest first-fault evidence is in generated `Move`: the
  `source[i] == 0` loop materializes `source + i * 4` once, then the next
  predicate path does `mov x9, x0`, immediately overwrites it with
  `mov x9, #4`, and loads from `4 + i * 4` instead of `source + i * 4`.
  The same pattern appears for `dest[j]`, overwriting `x1` with `#4`.

Most likely failing capability surface: AArch64 pointer-index address
materialization / selected-source publication for loads from function-parameter
pointers inside the recursive global-array path. This is adjacent to the prior
idea 13 `pointer/index arithmetic` family and does not currently point first
at global storage emission, recursion call preservation, test expectations, or
absent-selection fallback policy.

## Suggested Next

Execute Step 2 from `plan.md`: trace why the function-parameter pointer
element-load lowering in `Move` lost the base register after the prior idea 13
repair, then identify the owning AArch64 materialization/publication surface
before editing implementation code.

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

## Proof

Ran exactly:

```sh
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00181_c$'
```

Combined output is captured in `test_after.log`. Build was up to date
(`ninja: no work to do`); the targeted CTest subset ran one test and failed
`c_testsuite_aarch64_backend_src_00181_c` with `[RUNTIME_NONZERO]` /
`exit=Segmentation fault`. This proof is sufficient for the reproduction and
classification packet; it intentionally does not prove a fix.
