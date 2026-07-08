Status: Active
Source Idea Path: ideas/open/606_bir_global_initializer_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace The BIR Producer Boundary

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: traced the selected initializer bootstrap rows through HIR, BIR, LLVM-route, backend asm, and existing RV64 backend-object evidence. The owning boundary is the semantic BIR producer for global initializer facts, not prepared/global authority or RV64 object consumers.

Evidence created under `build/agent_state/606_step2_initializer_boundary/`:
- `commands_and_results.md`: command list, status summary, producer boundary notes, and row classification.
- Per-row artifacts named `<id>.hir.txt`, `<id>.hir.status`, `<id>.bir.txt`, `<id>.bir.status`, `<id>.llvm.ll`, `<id>.llvm.stderr.txt`, `<id>.llvm.status`, `<id>.backend.s`, `<id>.backend.stderr.txt`, and `<id>.backend.status` for `src_20040302-1.c`, `src_20021010-2.c`, `src_20021120-1.c`, `src_20010325-1.c`, `src_20010924-1.c`, `src_strlen-7.c`, `src_20020118-1.c`, and `src_ieee_fp-cmp-2.c`.

In-scope ordinary initializer rows:
- `src/20040302-1.c`: `--dump-hir-summary` succeeds (`functions=3 globals=2`), `--codegen llvm` succeeds and emits `@code = global [5 x i32] ...` plus `@__static_local_bar_1 = internal global [2 x ptr] [ptr blockaddress(...)]`; `--dump-bir` and backend asm both fail before prepared handoff with the semantic `lir_to_bir` bootstrap diagnostic for scalar integer/pointer globals, linear integer-array globals, and aggregate-backed globals with honest byte-address semantics.
- `src/20021010-2.c`: HIR and LLVM succeed; LLVM emits scalar pointer/integer globals plus struct aggregate globals `@global_bounds` and `@global_saveRect`; BIR/backend asm fail with the same semantic global-initializer bootstrap diagnostic.
- `src/20021120-1.c`: HIR and LLVM succeed; LLVM emits zero-initialized float/double array globals `@gd` and `@gf`; BIR/backend asm fail with the same semantic global-initializer bootstrap diagnostic.

Owning producer path:
- `src/backend/bir/lir_to_bir/module.cpp`: `lower_module(...)` iterates `context.lir_module.globals`, calls `lower_minimal_global(...)`, and emits the selected-row bootstrap note when global lowering returns empty.
- `src/backend/bir/lir_to_bir/globals.cpp`: `lower_minimal_global_impl(...)` owns the admitted global shapes and returns empty when the missing initializer fact cannot be represented as a BIR global with byte-addressable initializer elements, pointer offsets, layout, and linear-addressing metadata.
- `src/backend/backend.cpp`: `make_backend_dump_failure_message(...)`, `make_riscv_lir_handoff_failure_message(...)`, and `make_object_lowering_failure_message(...)` only wrap the producer note for dump, backend asm, and backend-object surfaces.

Missing in-scope fact:
- BIR must publish ordinary global initializer byte/aggregate facts before prepared/global handoff for the selected forms, including aggregate byte layout, initialized element bytes, pointer/function-address initializer metadata where present, and linear-addressing/storage-size facts. Step 3 should repair this semantic producer path across the selected family rather than matching testcase names.

LLVM route versus backend route:
- For all three selected rows, HIR and LLVM-route output succeeds, proving frontend parsing/lowering and LLVM IR generation can represent the initializer shapes.
- The backend-native BIR and asm routes fail before BIR/prepared handoff with the `lir_to_bir` bootstrap note, and the existing backend-object logs under `build/rv64_gcc_c_torture_backend/<case-id>/case.log` show the same object-route wrapper. That separates frontend/LLVM success from backend BIR bootstrap failure.

String-pool sentinel and excluded guards:
- `src/20010325-1.c`: HIR and LLVM succeed, but BIR/backend asm fail at `lower_string_constant_global(...)` with the separate byte-addressable string-pool constant diagnostic. Keep it as breadth context, not ordinary initializer progress.
- `src/20010924-1.c`: HIR, LLVM, and BIR succeed; backend-object fails later in prepared/global authority with `prepared selected object-data contract status=unsupported_but_coherent ... emitted_byte_count=0`.
- `src/strlen-7.c`: HIR, LLVM, and BIR succeed; backend-object fails later with `RV64 object route requires supported prepared global memory facts`. Backend asm also exposes an adjacent runtime external `strcpy` owner.
- `src/20020118-1.c`: HIR, LLVM, and BIR succeed; backend-object fails later in RV64/global consumption with `RV64 object route cannot emit prepared global symbol`.
- `src/ieee/fp-cmp-2.c`: HIR, LLVM, and BIR succeed; backend-object fails later in RV64/global consumption with `RV64 object route supports only 1-, 2-, 4-, and 8-byte prepared global memory accesses`.

## Suggested Next

Step 3 should implement the semantic BIR producer repair in `lower_minimal_global_impl(...)` / adjacent initializer-lowering helpers for the selected ordinary initializer subfamily, then write `build/agent_state/606_initializer_bootstrap_step3.allowlist` for the supervisor-delegated proof subset.

## Watchouts

- Keep prepared/global authority and RV64/global consumer failures separate from this BIR initializer bootstrap route; the Step 2 guard probes prove those rows already reach BIR and fail later.
- Do not pull string-library policy, runtime/link behavior, expectations, unsupported markers, allowlists, timeouts, or accounting into this plan.
- Reject named-case fixes, especially routes centered only on `src/20040302-1.c`.
- The current ordinary initializer proof set intentionally uses multiple same-diagnostic rows (`src/20040302-1.c`, `src/20021010-2.c`, `src/20021120-1.c`) so Step 3 cannot be accepted as a named-case-only route.
- Guard rows should remain downstream or adjacent failures after a BIR bootstrap repair; making them pass is not required for this plan and may indicate owner drift.
- The string-pool sentinel is a separate `lower_string_constant_global(...)` boundary and should not be claimed as ordinary aggregate/byte initializer progress unless the supervisor delegates that explicitly.

## Proof

No build or test run for this artifact-tracing packet; no `test_after.log` was produced.

Focused inspection commands were run for selected rows, sentinel, and guards:

```sh
./build/c4cll --dump-hir-summary <file>
./build/c4cll --dump-bir --target riscv64-linux-gnu <file>
./build/c4cll --codegen llvm --target riscv64-linux-gnu <file> -o build/agent_state/606_step2_initializer_boundary/<id>.llvm.ll
./build/c4cll --codegen asm --target riscv64-linux-gnu <file> -o build/agent_state/606_step2_initializer_boundary/<id>.backend.s
```

Results are recorded in `build/agent_state/606_step2_initializer_boundary/commands_and_results.md`; existing backend-object evidence was read from `build/rv64_gcc_c_torture_backend/<case-id>/case.log`.

Future delegated narrow proof command for code-changing packets:

```sh
cmake --build --preset default && ALLOWLIST=build/agent_state/606_initializer_bootstrap_step3.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```
