Status: Active
Source Idea Path: ideas/open/315_rv64_function_pointer_local_and_indirect_call_flow.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Function Pointer Residual Evidence

# Current Packet

## Just Finished

Step 1 function-pointer residual evidence normalization for idea 315.

Classified candidates:

- `src/00087.c`: emit `0`, link `0`, qemu `132`,
  `function_pointer_local_store_truncated`.
- `src/00124.c`: emit `0`, link `0`, qemu `139`,
  `function_pointer_return_and_indirect_call_residual`.

First bad mechanisms:

- `src/00087.c` stores `ptr @foo` into a local struct function-pointer field.
  Prepared BIR has `bir.store_local %lv.v.0, ptr @foo`, an indirect call plan,
  and direct-global address materializations for `@foo`, but emitted RV64
  assembly stops after the `main` prologue before any function-address
  materialization or local pointer store.
- `src/00124.c` returns a function pointer from `f1` and then calls the
  returned pointer. Emitted RV64 reaches the `return f2` branch but emits
  `mv a0, t0` without materializing `f2` into `t0`; the null-return path is
  also truncated before `main`, which contains chained indirect call plans.

Step 1 artifacts:

- `build/rv64_c_testsuite_probe_latest/triage_315_step1/probe_results.tsv`
- `build/rv64_c_testsuite_probe_latest/triage_315_step1/summary.md`

Selected first repair boundary:

- RV64 prepared function-address materialization for pointer-typed values:
  materialize direct function symbols (`@foo`, `@f1`, `@f2`) into pointer value
  homes/registers when stored, returned, or used as indirect callees, preserving
  prepared direct-global metadata.
- After that boundary, remaining work may include explicit indirect `jalr`
  call emission and pointer null-return handling if still failing.

## Suggested Next

Proceed to Step 2 by adding focused expected-repair backend coverage for the
selected boundary:

- local struct function-pointer field store followed by indirect call;
- function-pointer return feeding a chained indirect call, with a null pointer
  return path captured separately if needed.

## Watchouts

- Do not special-case filenames, symbol names, field offsets, or local
  aggregate shapes.
- Do not replace indirect calls with direct calls unless the function-pointer
  semantics are still genuinely preserved.
- `src/00087.c`'s prepared call plan already says `wrapper_kind=indirect`; do
  not prove it by emitting only `call foo`.
- `src/00124.c` has multiple gaps. Keep the next repair boundary on direct
  function-address materialization first, then re-evaluate indirect `jalr` and
  null pointer return handling with focused evidence.
- Keep external empty-stub policy, broad aggregate repair, and unrelated global
  data object work out of scope.

## Proof

Ran:

- `cmake --build --preset default -j`
- For `src/00087.c` and `src/00124.c`: `--dump-bir`, `--dump-prepared-bir`,
  RV64 asm emission, clang link, and qemu run into
  `build/rv64_c_testsuite_probe_latest/triage_315_step1/`
- `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Results:

- Build passed.
- `src/00087.c`: emit `0`, link `0`, qemu `132`,
  `function_pointer_local_store_truncated`.
- `src/00124.c`: emit `0`, link `0`, qemu `139`,
  `function_pointer_return_and_indirect_call_residual`.
- Delegated backend proof wrote `test_after.log` and failed with the existing
  `backend_riscv_prepared_edge_publication` failure as the only failing backend
  test.
