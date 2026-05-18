Status: Active
Source Idea Path: ideas/open/279_aarch64_c_testsuite_llvm_stack_helper_symbols.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Helper Origins And Ownership

# Current Packet

## Just Finished

Step 2 of `plan.md` traced the helper-symbol origin and owner layer for
`c_testsuite_aarch64_backend_src_00207_c`.

Trace by route layer:

- Frontend/HIR source shape: `tests/c/external/c-testsuite/src/00207.c`
  function `f1` has a VLA `char test[argc]` plus a backward `goto label;`.
  The VLA creates dynamic stack state, and the backward branch causes the
  stack to be restored before re-entering the label path.
- LIR origin: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` sets
  `module.need_stacksave` when a function has `ctx.vla_stack_save_ptr`.
  `src/codegen/lir/hir_to_lir/stmt.cpp` emits `LirStackRestoreOp` before a
  backward `goto` when `ctx.vla_stack_save_ptr` is present.
- BIR origin: AArch64 backend entry in `src/backend/backend.cpp` sets
  `BirLoweringOptions::preserve_dynamic_alloca = true`. With that option,
  `src/backend/bir/lir_to_bir/calling.cpp` lowers `LirStackSaveOp` and
  `LirStackRestoreOp` into `bir.call` records for `llvm.stacksave` and
  `llvm.stackrestore`; `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
  lowers counted `alloca` into `llvm.dynamic_alloca.<type>`.
- Semantic BIR evidence from the representative case:
  `%t0 = bir.call ptr llvm.stacksave()`,
  `%t3 = bir.call ptr llvm.dynamic_alloca.i8(i64 %t2)`, and two
  `bir.call void llvm.stackrestore(ptr %t0)` restore points in `@f1`.
- Prepared route evidence: `--dump-prepared-bir` records
  `prepared.summary @f1 ... has_dynamic_stack=yes ... dynamic_stack_ops=4`.
  `src/backend/prealloc/dynamic_stack.cpp` classifies the three helper call
  spellings into `PreparedDynamicStackOpKind::{StackSave, DynamicAlloca,
  StackRestore}`, and `src/backend/prealloc/frame_plan.cpp` marks the frame
  plan as `has_dynamic_stack`.
- AArch64 lowering/emission evidence: `src/backend/mir/aarch64/codegen/traversal.cpp`
  attaches the prepared dynamic-stack plan to `FunctionLoweringContext`, but
  `src/backend/mir/aarch64/codegen/dispatch.cpp` still routes these retained
  BIR helper calls through ordinary call lowering. `src/backend/mir/aarch64/codegen/calls.cpp`
  then builds direct-call machine nodes, and `print_call` prints them as
  `bl llvm.stacksave`, `bl llvm.dynamic_alloca.i8`, and
  `bl llvm.stackrestore`.

Owner decision: this is not a symbol/emitter validation problem alone. The
helpers represent real VLA/dynamic-stack semantics already preserved in the
prepared dynamic-stack plan. AArch64 lowering must either consume
`PreparedDynamicStackPlanFunction` and lower the stack-save, dynamic-alloca,
and stack-restore operations into supported AArch64 stack-pointer/frame
behavior, or truthfully reject the dynamic-stack family during AArch64 lowering
before unresolved `llvm.*` helper calls reach assembly/link output. Letting
ordinary direct-call lowering emit these helper names is the ownership bug.

Likely Step 3 focused proof file:
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`, with a
semantic prepared/BIR dynamic-stack case that currently emits or permits direct
calls to `llvm.stacksave`, `llvm.dynamic_alloca.i8`, and `llvm.stackrestore`.
The proof should be independent of `00207.c` and should require either concrete
AArch64 dynamic-stack lowering output or a truthful AArch64 lowering diagnostic
before machine output contains unresolved LLVM helper symbols.

Likely Step 4 implementation files:
`src/backend/mir/aarch64/codegen/dispatch.cpp`,
`src/backend/mir/aarch64/codegen/calls.cpp`, and, if choosing real lowering
rather than rejection, a focused AArch64 dynamic-stack lowering surface near
`src/backend/mir/aarch64/codegen/prologue.cpp` or a new adjacent codegen unit
that consumes `FunctionLoweringContext::dynamic_stack_plan`.

## Suggested Next

Run Step 3: add focused backend proof for the AArch64 dynamic-stack helper
family before implementing the repair.

Suggested backend proof command for Step 3:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

## Watchouts

- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not add filename-specific handling for `00207.c`.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Keep completed call-boundary move and `.LBB...` label fixes out of scope
  unless their exact old diagnostics return.
- Do not fix this by hiding `llvm.*` labels in the printer or by renaming helper
  symbols. The unresolved calls are a symptom of missing AArch64 dynamic-stack
  lowering or missing truthful rejection.
- Dynamic-stack preparation already records the intended helper facts; a Step 4
  implementation should consume that prepared authority instead of inferring
  behavior from the representative c-testsuite filename.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_00207_c$'; } 2>&1 | tee test_after.log
```

Result: configure/build succeeded; CTest selected
`c_testsuite_aarch64_backend_src_00207_c` and failed it as `[BACKEND_FAIL]`.
The command exits nonzero because the current AArch64 backend output still
leaves LLVM stack helper symbols unresolved for the assembler/link step:
`llvm.stacksave`, `llvm.dynamic_alloca.i8`, and `llvm.stackrestore`.
`test_after.log` is the preserved proof log.
