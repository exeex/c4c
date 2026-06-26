# 354 Reopen Classification - RV64 gcc_torture Backend Scan

Date: 2026-06-26
Source idea: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`

## Input Artifacts

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- per-case logs under `build/rv64_gcc_c_torture_backend/`

The available scan covers the full current allowlist:

- total: 1467
- pass: 211
- fail: 1256

No full rescan was run. The existing artifacts are internally complete enough
for lifecycle classification because every failed summary row points to a
per-case log and the top-level fail classes sum to 1256.

## Top-Level Failure Classes

- 1214 `RV64_C4C_OBJ_COMPILE_FAIL`
- 34 `RV64_BACKEND_RUNTIME_MISMATCH`
- 8 `RV64_C4C_OBJ_COMPILE_TIMEOUT`

The compile failures split into:

- 770 RV64 prepared-object route unsupported lowering/admission diagnostics
- 444 semantic `lir_to_bir` handoff diagnostics before prepared object handoff

## RV64 Prepared-Object Buckets Converted To Ideas

These 770 failures are current RV64 backend/object-route buckets and are
actionable enough to own as follow-up ideas:

- 385 unsupported instruction fragments:
  `unsupported_instruction_fragment`, representative `src/20000223-1.c`.
- 88 unsupported terminator fragments:
  `unsupported_terminator_fragment`, representative `src/20020206-2.c`.
- 92 unsupported move bundle target/copy shapes:
  `unsupported_move_bundle_target_shape` plus missing move bundle authority,
  representatives `src/20080519-1.c`, `src/20010604-1.c`, `src/930123-1.c`.
- 103 stack frame, callee-saved, parameter-home, and function-admission edges:
  representatives `src/930603-1.c`, `src/20001017-1.c`, `src/va-arg-21.c`.
- 30 global data and global-memory facts:
  representatives `src/20010924-1.c`, `src/20001121-1.c`,
  `src/20031211-1.c`, `src/pr57568.c`.
- 30 local memory addressing/access-width facts:
  representatives `src/20000722-1.c`, `src/20030910-1.c`.
- 42 scalar compare trunc and floating cast edges:
  representatives `src/20000313-1.c`, `src/20020225-2.c`.

## Runtime Bucket Converted To Idea

The 34 runtime mismatches are past object emission and link, so they are
current backend correctness failures even though the artifacts do not yet
prove a single lowering root cause:

- 23 `clang_exit=0 c4c_exit=Subprocess aborted`,
  representative `src/20000113-1.c`.
- 11 `clang_exit=0 c4c_exit=Segmentation fault`,
  representative `src/20070212-2.c`.

## Buckets Not Converted To Ideas In This Pass

The 444 semantic handoff failures were not converted to RV64 backend child
ideas in this pass. They fail before the prepared object handoff and need a
separate semantic/lir-to-bir owner if the supervisor wants to pursue them.
Their current split is:

- 268 local-memory semantic families: load/gep/store/scalar-local/alloca
- 55 call semantic families: direct-call and call-return
- 52 bootstrap global/string handoff limitations
- 34 runtime/intrinsic semantic families: memcpy and memset
- 16 scalar-binop semantic family
- 10 scalar-control-flow semantic family
- 9 function-signature semantic family

The 8 compile timeouts were also not converted. They are stable as a symptom
bucket but not yet assigned to RV64 backend versus frontend/semantic analysis
from the available logs:

- `src/930106-1.c`
- `src/990211-1.c`
- `src/arith-rand.c`
- `src/arith-rand-ll.c`
- `src/conversion.c`
- `src/pr49886.c`
- `src/pr51581-1.c`
- `src/pr51581-2.c`

## New Ideas Produced

- `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
- `ideas/open/396_rv64_object_route_terminator_fragment_lowering_refresh.md`
- `ideas/open/397_rv64_object_route_move_bundle_target_shapes.md`
- `ideas/open/398_rv64_object_route_stack_frame_and_param_home_edges.md`
- `ideas/open/399_rv64_object_route_global_data_and_symbol_memory.md`
- `ideas/open/400_rv64_object_route_local_memory_addressing_edges.md`
- `ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md`
- `ideas/open/402_rv64_gcc_torture_runtime_abort_and_segfault_mismatches.md`

Lifecycle decision: 354 remains incomplete after this pass because new child
ideas were produced and must be closed or intentionally superseded before the
umbrella can close again.
