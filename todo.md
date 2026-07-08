Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Alloca And Scalar-Local Proof Rows

# Current Packet

## Just Finished

Step 1 from `plan.md`: selected representative alloca and scalar/local-memory proof rows from the refreshed evidence set.

Refreshed evidence lives under `build/agent_state/605_step1_alloca_scalar_selection/`:
- `source_rows.tsv`
- `current_rows.tsv`
- `current_counts.tsv`

The refreshed source set has 65 total historical rows: 16 alloca rows and 49 scalar/local-memory rows. Current outcomes are:
- alloca source family: 16 fail, 0 success
- scalar/local-memory source family: 28 fail, 21 success

Selected in-scope alloca local-memory reps and expected pre-fix outcomes from `current_rows.tsv`:
- `src/20180921-1.c`: `alloca local-memory semantic family`
- `src/pr38151.c`: `alloca local-memory semantic family`
- `src/931004-10.c`: `alloca local-memory semantic family`

Selected in-scope scalar/local-memory reps and expected pre-fix outcomes from `current_rows.tsv`:
- `src/20020411-1.c`: `scalar/local-memory semantic family`
- `src/20041201-1.c`: `scalar/local-memory semantic family`
- `src/ffs-2.c`: `scalar/local-memory semantic family`

Selected guard rows and expected owners:
- `src/pr85169.c`: `load local-memory semantic family`
- `src/20050604-1.c`: `vector-binop semantic family`
- `src/20050607-1.c`: `scalar-cast semantic family`
- `src/ieee/compare-fp-1.c`: `unordered-float-compare scalar/local-memory semantic family`
- `src/20010605-2.c`: store-local guard
- `src/pr44468.c`: GEP-local guard
- `src/20040302-1.c`: global/bootstrap guard
- `src/20000722-1.c`: prepared/RV64 pointer-local guard
- `src/pr30778.c`: runtime/intrinsic guard
- `src/20020314-1.c`: ABI/stack-frame guard

## Suggested Next

Trace the selected six in-scope reps through HIR, semantic BIR, and LLVM-route output, then identify the shared alloca/scalar-local producer boundary before implementation.

## Watchouts

- Keep load, store, and GEP producer failures separate from this alloca/scalar-local route.
- Do not pull prepared/RV64 consumer, global initializer, ABI stack-frame, runtime, expectation, unsupported-marker, allowlist, timeout, or accounting work into this plan.
- Reject named-case fixes, especially routes centered only on `src/20180921-1.c`.
- `src/pr85169.c`, `src/20050604-1.c`, `src/20050607-1.c`, and `src/ieee/compare-fp-1.c` are current guard/downstream owners, not in-scope producer reps.

## Proof

No build run for this evidence-only Step 1 packet.

Supervisor verified the future proof regex resolves to 16 CTest names with `ctest -N`.

Delegated proof command for future Step 3/4 use:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20180921_1_c|llvm_gcc_c_torture_src_pr38151_c|llvm_gcc_c_torture_src_931004_10_c|llvm_gcc_c_torture_src_20020411_1_c|llvm_gcc_c_torture_src_20041201_1_c|llvm_gcc_c_torture_src_ffs_2_c|llvm_gcc_c_torture_src_pr85169_c|llvm_gcc_c_torture_src_20050604_1_c|llvm_gcc_c_torture_src_20050607_1_c|llvm_gcc_c_torture_src_ieee_compare_fp_1_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20000722_1_c|llvm_gcc_c_torture_src_pr30778_c|llvm_gcc_c_torture_src_20020314_1_c)$' >> test_after.log 2>&1
```
