Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Same-Family Breadth

# Current Packet

## Just Finished

Step 4 from `plan.md`: proved same-family breadth for all 65 Step 1 alloca/scalar-local evidence rows without implementation edits.

Evidence artifacts:
- `build/agent_state/605_step4_breadth/current_rows.tsv`: row-by-row comparison of Step 1 source family/status against fresh current `--dump-bir --target riscv64-linux-gnu` results.
- `build/agent_state/605_step4_breadth/current_counts.tsv`: current family/status counts.
- `build/agent_state/605_step4_breadth/moved_beyond_alloca.tsv`: rows whose original alloca local-memory producer stop moved to another owner.
- `build/agent_state/605_step4_breadth/remaining_in_scope_producers.tsv`: rows still stopped in alloca/scalar-local producer scope.
- `build/agent_state/605_step4_breadth/adjacent_downstream_rows.tsv`: rows now stopped in adjacent/downstream owners.
- `build/agent_state/605_step4_breadth/summary.md`: human-readable grouping summary plus per-row diagnostic text.
- Per-row fresh dump outputs live as `*.bir.txt` and `*.bir.err` in `build/agent_state/605_step4_breadth/`.

Current outcome counts across the 65 rows:
- `success`: 21 rows.
- `scalar/local-memory producer`: 22 rows.
- `load`: 9 rows.
- `unordered compare`: 7 rows.
- `store`: 2 rows.
- `GEP`: 1 row.
- `runtime`: 1 row.
- `scalar-cast`: 1 row.
- `vector`: 1 row.
- `alloca local-memory producer`, `prepared`, `global`, and `ABI`: 0 rows observed as current stops.

Rows moved beyond original alloca local-memory producer stops due to the Step 3 repair:
- Selected Step 3 rows: `src/20180921-1.c` -> `GEP`; `src/pr38151.c` -> `load`; `src/931004-10.c` -> `load`.
- Additional same-family rows: `src/20111208-1.c` -> `runtime`; `src/931004-12.c` -> `load`; `src/931004-14.c` -> `load`; `src/bswap-2.c` -> `store`; `src/pr42833.c` -> `load`; `src/pr82524.c` -> `store`; `src/strct-stdarg-1.c` -> `load`; `src/zero-struct-1.c` -> `load`.

Rows still stopped in alloca/scalar-local producer scope:
- `src/20020411-1.c`, `src/20041201-1.c`, `src/20070614-1.c`, `src/complex-2.c`, `src/complex-5.c`, `src/complex-6.c`, `src/ffs-2.c`, `src/20020227-1.c`, `src/20021118-2.c`, `src/921013-1.c`, `src/961223-1.c`, `src/builtin-bitops-1.c`, `src/ffs-1.c`, `src/ieee/acc1.c`, `src/ieee/acc2.c`, `src/pr42248.c`, `src/pr42691.c`, `src/pr47538.c`, `src/pr56837.c`, `src/pr61725.c`, `src/scal-to-vec1.c`, `src/scal-to-vec2.c`.

Adjacent-owner/downstream rows:
- `GEP`: `src/20180921-1.c`.
- `load`: `src/pr38151.c`, `src/pr85169.c`, `src/931004-10.c`, `src/931004-12.c`, `src/931004-14.c`, `src/pr42833.c`, `src/stdarg-4.c`, `src/strct-stdarg-1.c`, `src/zero-struct-1.c`.
- `store`: `src/bswap-2.c`, `src/pr82524.c`.
- `vector`: `src/20050604-1.c`.
- `scalar-cast`: `src/20050607-1.c`.
- `unordered compare`: `src/ieee/compare-fp-1.c`, `src/ieee/compare-fp-3.c`, `src/ieee/compare-fp-4.c`, `src/ieee/fp-cmp-4.c`, `src/ieee/fp-cmp-4f.c`, `src/ieee/fp-cmp-4l.c`, `src/ieee/fp-cmp-5.c`.
- `runtime`: `src/20111208-1.c`.
- `prepared`, `global`, and `ABI`: none observed in this Step 4 BIR-dump classification.

## Suggested Next

Execute Step 5 closure readiness. Recommended stance: the alloca-local carrier-name repair has enough breadth for closure because 11 original alloca producer stops moved beyond that family, including all 3 selected reps and 8 additional same-family rows, while adjacent load/GEP/store/vector/scalar-cast/unordered-compare/runtime boundaries stayed separate. Remaining scalar/local-memory producer rows are a follow-up initiative rather than Step 3 alloca repair fallout.

## Watchouts

Do not pull the observed downstream `load`, `GEP`, `store`, `vector`, `scalar-cast`, unordered compare, or runtime stops into the alloca-local repair. They are preserved owner boundaries for Step 5 closure text.

The 22 remaining producer rows are all scalar/local-memory current stops; no current row remains stopped in `alloca local-memory semantic family`.

## Proof

Proof log: `test_after.log`.

Delegated proof command run successfully:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20020411_1_c|llvm_gcc_c_torture_src_20041201_1_c|llvm_gcc_c_torture_src_20070614_1_c|llvm_gcc_c_torture_src_20180921_1_c|llvm_gcc_c_torture_src_complex_2_c|llvm_gcc_c_torture_src_complex_5_c|llvm_gcc_c_torture_src_complex_6_c|llvm_gcc_c_torture_src_ffs_2_c|llvm_gcc_c_torture_src_pr38151_c|llvm_gcc_c_torture_src_pr85169_c|llvm_gcc_c_torture_src_20000519_1_c|llvm_gcc_c_torture_src_20020227_1_c|llvm_gcc_c_torture_src_20021118_2_c|llvm_gcc_c_torture_src_20050604_1_c|llvm_gcc_c_torture_src_20050607_1_c|llvm_gcc_c_torture_src_20071213_1_c|llvm_gcc_c_torture_src_20111208_1_c|llvm_gcc_c_torture_src_921013_1_c|llvm_gcc_c_torture_src_931004_10_c|llvm_gcc_c_torture_src_931004_12_c|llvm_gcc_c_torture_src_931004_14_c|llvm_gcc_c_torture_src_961223_1_c|llvm_gcc_c_torture_src_bswap_2_c|llvm_gcc_c_torture_src_builtin_bitops_1_c|llvm_gcc_c_torture_src_complex_7_c|llvm_gcc_c_torture_src_ffs_1_c|llvm_gcc_c_torture_src_fprintf_chk_1_c|llvm_gcc_c_torture_src_ieee_acc1_c|llvm_gcc_c_torture_src_ieee_acc2_c|llvm_gcc_c_torture_src_ieee_compare_fp_1_c|llvm_gcc_c_torture_src_ieee_compare_fp_3_c|llvm_gcc_c_torture_src_ieee_compare_fp_4_c|llvm_gcc_c_torture_src_ieee_fp_cmp_4_c|llvm_gcc_c_torture_src_ieee_fp_cmp_4f_c|llvm_gcc_c_torture_src_ieee_fp_cmp_4l_c|llvm_gcc_c_torture_src_ieee_fp_cmp_5_c|llvm_gcc_c_torture_src_ieee_inf_2_c|llvm_gcc_c_torture_src_ieee_inf_3_c|llvm_gcc_c_torture_src_ieee_pr36332_c|llvm_gcc_c_torture_src_pr42248_c|llvm_gcc_c_torture_src_pr42691_c|llvm_gcc_c_torture_src_pr42833_c|llvm_gcc_c_torture_src_pr47538_c|llvm_gcc_c_torture_src_pr56837_c|llvm_gcc_c_torture_src_pr61725_c|llvm_gcc_c_torture_src_pr64006_c|llvm_gcc_c_torture_src_pr68381_c|llvm_gcc_c_torture_src_pr71554_c|llvm_gcc_c_torture_src_pr82524_c|llvm_gcc_c_torture_src_pr84169_c|llvm_gcc_c_torture_src_pr85095_c|llvm_gcc_c_torture_src_pr89434_c|llvm_gcc_c_torture_src_printf_chk_1_c|llvm_gcc_c_torture_src_scal_to_vec1_c|llvm_gcc_c_torture_src_scal_to_vec2_c|llvm_gcc_c_torture_src_stdarg_4_c|llvm_gcc_c_torture_src_strct_stdarg_1_c|llvm_gcc_c_torture_src_user_printf_c|llvm_gcc_c_torture_src_va_arg_20_c|llvm_gcc_c_torture_src_va_arg_9_c|llvm_gcc_c_torture_src_vfprintf_1_c|llvm_gcc_c_torture_src_vfprintf_chk_1_c|llvm_gcc_c_torture_src_vprintf_1_c|llvm_gcc_c_torture_src_vprintf_chk_1_c|llvm_gcc_c_torture_src_zero_struct_1_c)$' >> test_after.log 2>&1
```

Result: build passed; broad same-family CTest subset passed `65/65` with `100% tests passed, 0 tests failed out of 65`.

Fresh BIR dump classification also completed for the same 65 rows under `build/agent_state/605_step4_breadth/`.
