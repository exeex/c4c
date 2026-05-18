# AArch64 Function Pointer Indirect Call Values Todo

Status: Active
Source Idea Path: ideas/open/289_aarch64_function_pointer_indirect_call_values.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Minimal Local Function-Pointer Failure

# Current Packet

## Just Finished

Completed Plan Step 1, "Establish Minimal Local Function-Pointer Failure", for
`tests/c/external/c-testsuite/src/00087.c`.

Observed baseline failure:
- The delegated AArch64 proof builds `c4cll` and fails only at runtime:
  `c_testsuite_aarch64_backend_src_00087_c` exits with `Bus error`.
- Generated AArch64 frame shape is aligned: `main` starts with
  `sub sp, sp, #32`, saves `x30` at `[sp, #16]`, `x20` at `[sp, #8]`, and
  `x19` at `[sp]`.
- The generated binary materializes `foo` as a real function symbol:
  `foo` is global FUNC at `0x768`, size 8, and `main` is global FUNC at
  `0x770`, size 40.
- The final `main` assembly has no local aggregate field store/load sequence
  for `v.fptr`: after the prologue it immediately executes `blr x20`, then
  moves `x0` into `x13` and returns.
- The indirect `blr` callee register is `x20`. `x20` is only saved to
  `[sp, #8]` before the call and restored after the call; no instruction writes
  the `foo` address into `x20` before `blr`.
- LLVM output before the native backend is semantically correct:
  `alloca %struct.S`, GEP field 0, `store ptr @foo`, GEP field 0, `load ptr`,
  then `call i32 () %t2()`.
- HIR is also semantically correct: `decl v: struct S`,
  `(v#L0.fptr{owner=S} = foo)`, and `return v#L0.fptr{owner=S}()`.
- Semantic BIR condenses the local path to `bir.store_local %lv.v.0, ptr @foo`
  and an indirect call printed as `bir.call i32 @foo()`.
- Prepared BIR records the call as indirect:
  `wrapper=indirect`, `indirect_callee=@foo`, `indirect_home=register`,
  `indirect_bank=gpr`, `indirect_value_id=0`, `indirect_reg=x20`.
- Prepared storage records `@foo` as `value_id=0` in register `x20`; prepared
  addressing records the local store at `block=entry inst_index=0` as
  `base=frame_slot stored=@foo frame_slot=#0 offset=0 size=8 align=8`.
- No before-call move bundle or address materialization writes the symbol
  address into `x20`, and the final assembly emits neither the prepared
  frame-slot store nor a reload before the indirect call.

## Suggested Next

Delegate Step 2 to implement the semantic backend repair for AArch64 function
pointer values carried through local storage into indirect calls. The smallest
next packet should preserve the current LLVM/HIR behavior and fix the native
path so symbol pointer `@foo` is materialized into the prepared indirect callee
home, or stored to/reloaded from the prepared local frame slot, before `blr`.

## Watchouts

- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- Reject named-case shortcuts, exact function-name shortcuts, aggregate-field
  shortcuts, and emitted-instruction-text matching.
- Do not treat this as residual stack-frame/SP alignment unless a new
  unaligned frame is proven.
- Preserve direct-call behavior and existing aligned frame behavior.
- Keep non-bus runtime failures from the wider sample outside this route unless
  they are proven to share the same function-pointer indirect-call owner.
- Keep parser/sema, broad aggregate ABI, compare/branch lowering, libc
  behavior, and timeout-sensitive cases out of this route.
- Likely Step 2 surfaces are:
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` for local pointer
  store/load BIR preservation, `src/backend/prealloc/call_plans.cpp` for
  indirect callee planning of symbol pointer values, and
  `src/backend/mir/aarch64/codegen/memory.cpp`,
  `src/backend/mir/aarch64/codegen/globals.cpp`, and
  `src/backend/mir/aarch64/codegen/calls.cpp` for AArch64 symbol-address
  materialization, local store emission, and `blr` register readiness.
- Avoid treating this as a direct-call conversion. The source expression is an
  indirect call through a function pointer field, and Step 2 should preserve
  that semantic shape instead of rewriting only this case to `bl foo`.

## Proof

Ran the delegated proof exactly:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan --output-on-failure -R '^c_testsuite_aarch64_backend_src_00087_c$'; } > test_before.log 2>&1
```

Result: failed as expected; `test_before.log` records
`[RUNTIME_NONZERO] .../src/00087.c exit=Bus error`.

Transient inspection artifacts used:
- `/tmp/00087.hir`
- `/tmp/00087.ll`
- `/tmp/00087.bir`
- `/tmp/00087.prepared-bir`
- `/tmp/00087.mir`
