Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace The Producer Boundary

# Current Packet

## Just Finished

Step 2 from `plan.md`: traced the selected alloca and scalar/local-memory reps through HIR, semantic BIR, LLVM-route output, and focused MIR/trace attempts.

Inspection artifacts live under `build/agent_state/605_step2_trace/`:
- `summary.tsv`
- `focused_mir_summary.tsv`
- one directory per selected or guard row with `hir.*`, `bir.*`, and `llvm.*`; the six selected reps also have `mir.*` and `trace_mir.*`.

All six selected reps pass HIR and LLVM-route generation, but semantic BIR fails before prepared/RV64 handoff:
- `src/20180921-1.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `aw` in `alloca local-memory semantic family`.
- `src/pr38151.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `check2848va` in `alloca local-memory semantic family`.
- `src/931004-10.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `f` in `alloca local-memory semantic family`.
- `src/20020411-1.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `foo` in `scalar/local-memory semantic family`.
- `src/20041201-1.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `checkScc2` in `scalar/local-memory semantic family`.
- `src/ffs-2.c`: HIR `0`, BIR `1`, LLVM `0`; latest BIR failure is function `main` in `scalar/local-memory semantic family`.

Owning producer paths:
- The alloca rows fail through `src/backend/bir/lir_to_bir/memory/coordinator.cpp`: `BirFunctionLowerer::lower_scalar_or_local_memory_inst`, on the `LirAllocaOp` arm, which delegates to `src/backend/bir/lir_to_bir/memory/local_slots.cpp`: `BirFunctionLowerer::lower_local_memory_alloca_inst` and emits `alloca local-memory semantic family` if that producer cannot publish the local slot/object fact.
- The scalar/local rows fail through `src/backend/bir/lir_to_bir/module.cpp`: `BirFunctionLowerer::lower_block_insts`, which wraps an unlowered block instruction from `lower_scalar_or_local_memory_inst` as `scalar/local-memory semantic family`.
- HIR and LLVM-route success on all six selected rows rules out frontend parsing/HIR and basic LLVM-route generation as the first boundary. MIR/trace attempts also stop at the same semantic BIR producer boundary, before prepared/RV64 handoff.

Guard rows remain excluded from Step 3 implementation ownership:
- `src/pr85169.c`: `load local-memory semantic family`, owned by the load producer route.
- `src/20050604-1.c`: `vector-binop semantic family`, owned by vector scalar lowering.
- `src/20050607-1.c`: `scalar-cast semantic family`, owned by scalar cast lowering.
- `src/ieee/compare-fp-1.c`: `unordered-float-compare scalar/local-memory semantic family`, owned by unordered float compare lowering.
- `src/20010605-2.c`: Step 1 store-local guard; not part of Step 3 alloca/scalar producer ownership.
- `src/pr44468.c`: `gep local-memory semantic family`, owned by the GEP producer route.
- `src/20040302-1.c`: bootstrap/global lowering guard, not local alloca/scalar producer ownership.
- `src/20000722-1.c`: already reaches semantic BIR/LLVM in this trace and remains a prepared/RV64 pointer-local guard.
- `src/pr30778.c`: already reaches semantic BIR/LLVM in this trace and remains runtime/intrinsic-adjacent guard evidence.
- `src/20020314-1.c`: already reaches semantic BIR/LLVM in this trace and remains ABI/stack-frame guard evidence.

## Suggested Next

Execute Step 3 as a narrow alloca-local producer repair first, targeting the shared `LirAllocaOp` path in `lower_local_memory_alloca_inst` for the three alloca rows while preserving guard ownership. Treat the three scalar/local rows as a sibling producer subfamily unless the alloca repair naturally exposes the same semantic fact without widening.

## Watchouts

The selected rows split into two real in-scope producer subfamilies rather than one mixed implementation surface: alloca-local production and scalar/local block-instruction production. Step 3 should avoid bundling both unless one general local-memory fact repair demonstrably advances both groups.

Do not pull load/store/GEP, unordered float compare, scalar-cast, vector-binop, bootstrap/global, prepared/RV64, ABI, runtime, expectation, unsupported-marker, allowlist, timeout, or accounting work into the Step 3 repair.

## Proof

No build run for this evidence-only Step 2 packet. No root logs were touched.

Commands run for the selected reps and guard rows:

```sh
build/c4cll --dump-hir tests/c/external/gcc_torture/<row>
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
build/c4cll --codegen llvm --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
```

Additional commands run for the six selected reps:

```sh
build/c4cll --dump-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
build/c4cll --trace-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/<row>
```

Supervisor-verified future Step 3/4 proof command remains:

Delegated proof command for future Step 3/4 use:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20180921_1_c|llvm_gcc_c_torture_src_pr38151_c|llvm_gcc_c_torture_src_931004_10_c|llvm_gcc_c_torture_src_20020411_1_c|llvm_gcc_c_torture_src_20041201_1_c|llvm_gcc_c_torture_src_ffs_2_c|llvm_gcc_c_torture_src_pr85169_c|llvm_gcc_c_torture_src_20050604_1_c|llvm_gcc_c_torture_src_20050607_1_c|llvm_gcc_c_torture_src_ieee_compare_fp_1_c|llvm_gcc_c_torture_src_20010605_2_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20000722_1_c|llvm_gcc_c_torture_src_pr30778_c|llvm_gcc_c_torture_src_20020314_1_c)$' >> test_after.log 2>&1
```
