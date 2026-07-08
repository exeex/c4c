Status: Active
Source Idea Path: ideas/open/604_bir_local_memory_gep_address_semantics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace the GEP Producer Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: traced the selected GEP representatives to the BIR semantic producer boundary.

Trace commands captured under `build/agent_state/604_step2_trace/`:

```sh
./build/c4cll --dump-hir tests/c/external/gcc_torture/src/pr80421.c
./build/c4cll --dump-hir tests/c/external/gcc_torture/src/930614-2.c
./build/c4cll --dump-hir tests/c/external/gcc_torture/src/pr24851.c
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr80421.c
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/930614-2.c
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr24851.c
./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/pr80421.c
./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/930614-2.c
./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/pr24851.c
```

Results:
- HIR dumps and LLVM-route output succeed for all three direct representatives.
- Semantic BIR dumps fail before prepared/RV64 handoff for all three direct representatives.
- First visible producer diagnostic:
  - `src/pr80421.c`: `semantic lir_to_bir function 'bar' failed in gep local-memory semantic family`.
  - `src/930614-2.c`: `semantic lir_to_bir function 'main' failed in gep local-memory semantic family`.
  - `src/pr24851.c`: `semantic lir_to_bir function 'main' failed in gep local-memory semantic family`.

Owning BIR producer path:
- Dispatch owner: `src/backend/bir/lir_to_bir/memory/coordinator.cpp`, `BirFunctionLowerer::lower_local_memory_inst`, `LirGepOp` branch.
- Producer owner: `src/backend/bir/lir_to_bir/memory/addressing.cpp`, `BirFunctionLowerer::lower_memory_gep_inst`.
- Diagnostic source: `lower_memory_gep_inst` records `note_function_lowering_family_failure("gep local-memory semantic family")`, then `lower_local_memory_inst` reports the same GEP family on failure.

Missing fact:
- BIR needs a direct local-object GEP/address producer contract that publishes local source object identity, derivation path, byte offset/layout, element/range coordinates, and the resulting local-address provenance before prepared/RV64 lowering.
- The direct reps are ordinary local object shapes:
  - `src/pr80421.c`: local stack array `char c[]`, direct base `c + 390`, then derived pointer updates and dynamic `f[g]` access.
  - `src/930614-2.c`: local multidimensional array `x[i][k][j][l]` with loop-carried dynamic indices.
  - `src/pr24851.c`: local array base `&a[1]`, then in-object negative subscript `&q[-1]`.

Boundary separation:
- This is not pointer-value freshness: the failing path has not yet published the local object address facts for these direct local objects.
- This is not selected base+offset authority: no prepared/RV64 selected-address consumer is reached.
- This is not RV64 frame-slot consumption: `--dump-bir` fails before prepared BIR and before target frame-slot materialization.
- This is not a global/static object, runtime/string intrinsic, aggregate/member/flexible-layout, variadic, or pointer/formal provenance repair.

Rows excluded from Step 3 implementation ownership:
- Pointer/formal provenance guards: `src/pr44468.c`, `src/pr65956.c`, `src/pr58209.c`.
- Global/static object GEP guards: `src/20000717-4.c`, `src/20031214-1.c`, `src/20080424-1.c`.
- Runtime/string intrinsic guards: `src/memcpy-2.c`, `src/strlen-1.c`.
- Aggregate/member/flexible-layout or alias guards: `src/20051113-1.c`, `src/20100430-1.c`, `src/pta-field-1.c`.
- Variadic guard: `src/va-arg-22.c`.
- Adjacent pointer local-memory consumer guard: `src/20000722-1.c`.

## Suggested Next

Execute Step 3 from `plan.md` only for the BIR local-memory GEP/address producer contract for direct local objects. The implementation should make the direct local object shapes publish explicit local address/provenance facts before prepared/RV64 consumption; if that contract cannot be stated without pointer/formal provenance, global/static object authority, broad pointer arithmetic policy, or target-side reconstruction, stop and report a blocker/lifecycle issue.

## Watchouts

- Keep BIR local-memory GEP/address production separate from RV64 frame-slot consumption and target-side pointer authority.
- Do not broaden unsupported pointer arithmetic policy under this idea.
- Do not edit expectations, unsupported markers, allowlists, timeout, accounting, ABI, runtime, or global-data behavior.
- Treat named rows only as probes; reject testcase-shaped shortcuts.
- The refreshed artifact count is `62`, not the source idea's historical `43` estimate.
- Step 3 should start from direct local-object GEP admission only; formal pointer provenance, global/static objects, runtime/string intrinsics, aggregate/member/flexible-layout or alias cases, variadic routing, and pointer local-memory consumers are guards.
- The LLVM-route evidence shows the direct reps have ordinary `alloca`-rooted `getelementptr` shapes, but the backend route must not implement a raw textual LLVM-GEP shortcut. It needs the semantic BIR producer fact.

## Proof

Evidence-only/todo-only packet; no build or test run required. No root-level proof log was created.

Inspection artifacts:
- `build/agent_state/604_step2_trace/pr80421.c.hir.txt`
- `build/agent_state/604_step2_trace/pr80421.c.bir.txt`
- `build/agent_state/604_step2_trace/pr80421.c.llvm.ll`
- `build/agent_state/604_step2_trace/930614-2.c.hir.txt`
- `build/agent_state/604_step2_trace/930614-2.c.bir.txt`
- `build/agent_state/604_step2_trace/930614-2.c.llvm.ll`
- `build/agent_state/604_step2_trace/pr24851.c.hir.txt`
- `build/agent_state/604_step2_trace/pr24851.c.bir.txt`
- `build/agent_state/604_step2_trace/pr24851.c.llvm.ll`

Supervisor-delegated proof command for future Step 3/4 use:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_pr80421_c|llvm_gcc_c_torture_src_930614_2_c|llvm_gcc_c_torture_src_pr24851_c|llvm_gcc_c_torture_src_pr44468_c|llvm_gcc_c_torture_src_pr65956_c|llvm_gcc_c_torture_src_pr58209_c|llvm_gcc_c_torture_src_20000717_4_c|llvm_gcc_c_torture_src_20031214_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_memcpy_2_c|llvm_gcc_c_torture_src_strlen_1_c|llvm_gcc_c_torture_src_20051113_1_c|llvm_gcc_c_torture_src_20100430_1_c|llvm_gcc_c_torture_src_pta_field_1_c|llvm_gcc_c_torture_src_va_arg_22_c|llvm_gcc_c_torture_src_20000722_1_c)$' >> test_after.log 2>&1
```
