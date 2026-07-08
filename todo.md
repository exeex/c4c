Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Same-Family Breadth

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: reran the selected proof command and refreshed `--dump-bir --target riscv64-linux-gnu` classification across the 62 Step 1 rows in `build/agent_state/499_step1_gep_local_memory_classification/rows.tsv`.

Fresh classification artifact:
- `build/agent_state/604_step4_gep_breadth/current_rows.tsv`
- `build/agent_state/604_step4_gep_breadth/current_counts.tsv`

Same-family movement after the accepted Step 3 commits:
- Full-row semantic BIR successes: `21/62`.
- Direct local-object candidates: `src/pr24851.c` and `src/930614-2.c` now succeed through semantic BIR; `src/pr80421.c` moved past the old GEP producer and now stops at `load local-memory semantic family`.
- Additional full-row successes came from guard-class rows whose current BIR dump no longer exposes the old GEP producer stop: `src/20000717-4.c`, `src/20021011-1.c`, `src/20031214-1.c`, `src/20031215-1.c`, `src/20051104-1.c`, `src/20120808-1.c`, `src/20120919-1.c`, `src/950426-1.c`, `src/builtin-prefetch-4.c`, `src/builtin-prefetch-5.c`, `src/longlong.c`, `src/pr36038.c`, `src/pr38051.c`, `src/pr58209.c`, `src/pr80153.c`, `src/pr90025.c`, `src/strcpy-1.c`, `src/string-opt-17.c`, and `src/strlen-1.c`.
- Downstream non-GEP stops without full BIR success: `src/pr80421.c` and `src/20030928-1.c` now stop at `load local-memory semantic family`; `src/memcpy-2.c`, `src/memset-1.c`, `src/memset-2.c`, `src/memset-3.c`, and `src/string-opt-5.c` now stop at explicit runtime/intrinsic owners.
- Remaining rows still reporting `gep local-memory semantic family`: `34/62`.

Remaining GEP stops by Step 1 guard class:
- `aggregate_member_flexible_or_alias_boundary`: `10` rows: `src/20001203-2.c`, `src/20051113-1.c`, `src/20060412-1.c`, `src/20070824-1.c`, `src/20071120-1.c`, `src/20100430-1.c`, `src/mayalias-2.c`, `src/pr41395-1.c`, `src/pta-field-1.c`, `src/pta-field-2.c`.
- `global_or_static_object_gep_boundary`: `2` rows: `src/20080424-1.c`, `src/ieee/copysign2.c`.
- `pointer_or_formal_provenance_boundary`: `19` rows: `src/pr44468.c`, `src/pr48571-1.c`, `src/pr65956.c`, `src/20001027-1.c`, `src/20010116-1.c`, `src/20010910-1.c`, `src/20030717-1.c`, `src/20080604-1.c`, `src/20090113-1.c`, `src/20100708-1.c`, `src/920922-1.c`, `src/990128-1.c`, `src/pr38048-2.c`, `src/pr39339.c`, `src/pr49419.c`, `src/pr52760.c`, `src/pr65401.c`, `src/pr71083.c`, `src/pr86844.c`.
- `runtime_or_string_intrinsic_boundary`: `2` rows: `src/20011121-1.c`, `src/strlen-5.c`.
- `variadic_boundary`: `1` row: `src/va-arg-22.c`.

## Suggested Next

Move to Step 5 final proof summary and closure-readiness review. No runbook refinement looks required before Step 5 for the direct local-object GEP producer goal: the three direct candidates either succeed or have moved to a downstream owner, and the remaining GEP stops are still concentrated in Step 1 guard/prerequisite groups rather than a broader direct-local subfamily.

## Watchouts

- Treat the `21/62` success count as refreshed `--dump-bir` evidence only; it is not a full backend-object pass count.
- Keep `src/pr80421.c` and `src/20030928-1.c` with downstream `load local-memory` ownership unless the supervisor explicitly opens a consumer packet.
- Do not pull the remaining pointer/formal, global/static, aggregate/member/flexible/alias, runtime/string, or variadic guard rows into the direct local-object GEP producer route without plan review.
- The remaining `34/62` GEP stops are not a single proven in-scope Step 3 miss; they preserve Step 1 guard-owner boundaries.

## Proof

Ran the supervisor-delegated Step 4 proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```

Result: build passed; CTest passed `16/16`; proof log is `test_after.log`.

Also reran fresh semantic BIR dumps for all 62 Step 1 rows:

```sh
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
```

Result: `21/62` success, `34/62` still `gep local-memory semantic family`, `2/62` downstream `load local-memory semantic family`, and `5/62` explicit runtime/intrinsic owner stops.
