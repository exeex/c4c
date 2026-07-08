Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Representative GEP-Family Proof Rows

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: selected representative GEP-family proof rows from `build/agent_state/499_step1_gep_local_memory_classification/`.

Refreshed evidence:
- The Step 1 artifact reproduces `62` rows where `semantic_family` is `gep local-memory semantic family`.
- The source idea's `43`-row breadth remains historical context; use the refreshed `62`-row artifact as the current selection basis.

Selected direct local-object GEP representatives:
- `src/pr80421.c`: local stack array `char c[]`, pointer `f = c + 390`, dynamic indexed loads through `f[g]`.
- `src/930614-2.c`: local multidimensional array `x[i][k][j][l]` with loop-carried indices.
- `src/pr24851.c`: local array negative subscript through `q = &a[1]` and `p = &q[-1]`.

Fail-closed guard groups:
- Pointer/formal provenance: `src/pr44468.c`, `src/pr65956.c`, `src/pr58209.c`.
- Global/static object GEP: `src/20000717-4.c`, `src/20031214-1.c`, `src/20080424-1.c`.
- Runtime/string intrinsic: `src/memcpy-2.c`, `src/strlen-1.c`.
- Aggregate/member/flexible or alias: `src/20051113-1.c`, `src/20100430-1.c`, `src/pta-field-1.c`.
- Variadic: `src/va-arg-22.c`.
- Adjacent pointer local-memory consumer guard: `src/20000722-1.c`.

Expected pre-fix outcome:
- The three direct representatives currently report `gep local-memory semantic family`.
- Guard rows should keep their adjacent/fail-closed owners and must not be pulled into the direct local-object GEP repair.

## Suggested Next

Execute Step 2 from `plan.md`: trace the selected representatives to the first BIR GEP/address producer boundary, identify the missing local-address fact, and confirm each guard row remains outside Step 3 implementation ownership.

## Watchouts

- Keep BIR local-memory GEP/address production separate from RV64 frame-slot consumption and target-side pointer authority.
- Do not broaden unsupported pointer arithmetic policy under this idea.
- Do not edit expectations, unsupported markers, allowlists, timeout, accounting, ABI, runtime, or global-data behavior.
- Treat named rows only as probes; reject testcase-shaped shortcuts.
- The refreshed artifact count is `62`, not the source idea's historical `43` estimate.
- Step 3 should start from direct local-object GEP admission only; formal pointer provenance, global/static objects, runtime/string intrinsics, aggregate/member/flexible-layout or alias cases, variadic routing, and pointer local-memory consumers are guards.

## Proof

Evidence-only/todo-only packet; no build or test run required.

Supervisor-delegated proof command for future Step 3/4 use:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```
