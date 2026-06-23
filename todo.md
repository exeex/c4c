Status: Active
Source Idea Path: ideas/open/326_rv64_variadic_target_llvm_route_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Current RV64 Variadic Fact Boundaries

# Current Packet

## Just Finished

Step 1 located the current RV64 variadic LLVM-route/LIR fact boundaries and refreshed the delegated triage LLVM outputs:

- `build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll`
- `build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll`
- `build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll`

Selected code surfaces:

- Direct variadic call target/signature facts: `src/codegen/lir/hir_to_lir/call/target.cpp` builds `LirCallSignature`, records unresolved external declarations, and chooses call return spelling.
- Direct variadic call argument facts: `src/codegen/lir/hir_to_lir/call/args.cpp` applies default argument promotion for variadic operands and returns `PreparedCallArg`/`LirCallArg` without target extension attributes today.
- Call fact carrier and rendering: `src/codegen/lir/ir.hpp` has `LirCallSignature`, `LirCallArg`, `LirExternDecl`, and `LirCallOp`; `src/codegen/lir/call_args_ops.hpp` mirrors structured args; `src/codegen/lir/lir_printer.cpp` prints declarations and call sites without `signext`/`zeroext` attribute fields today.
- `va_list` layout and storage policy: `src/codegen/shared/llvm_helpers.hpp` currently treats only Apple AArch64 as pointer-object `va_list`; RV64 Linux falls through to `%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }`.
- `va_arg` lowering: `src/codegen/lir/hir_to_lir/call/vaarg.cpp` routes pointer-object targets through `LirVaArgOp`, AMD64 through `emit_amd64_va_arg`, and all non-pointer-object non-AMD64 scalar integer/pointer cases through the AArch64 structured `emit_aarch64_vaarg_gp_src_ptr` path.
- `va_start` and local `va_list` storage surface: `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` emits `%struct.__va_list_tag_` declarations and allocas when `llvm_va_list_is_pointer_object(target)` is false; `src/codegen/lir/lir_printer.cpp` prints `llvm.va_start`/`llvm.va_end`.

Observed current IR shapes:

- RV64 direct `printf`: emits target triple `riscv64-unknown-linux-gnu`, incorrectly emits `%struct.__va_list_tag_`, declares `declare i32 @printf(ptr, ...)`, and calls `%t1 = call i32 (ptr, ...) @printf(ptr %t0, i32 7)`. Missing expected RV64 `signext` on the `i32` declaration return/call result and on integer variadic operand `7`.
- RV64 scalar `stdarg`: emits the same structured `%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }` route as AArch64, allocates `%lv.ap = alloca %struct.__va_list_tag_`, calls `llvm.va_start`, loads field 3 as an offset, branches through `vaarg.regtry`/`vaarg.reg`/`vaarg.stack`, updates `gr_offs`, and uses a phi between register-save and stack paths. This is not the clang-observed RV64 pointer cursor.
- AArch64 scalar `stdarg`: currently emits the structured `%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }` route with the same `gr_offs`/stack branching shape. This is the shape Step 3 must preserve for AArch64 while removing it from RV64.

## Suggested Next

Execute Step 2 first as a narrow call-attribute packet:

Objective: publish RV64 direct external variadic integer extension facts in the LLVM-route output without changing `stdarg` lowering yet.

Owned likely implementation files: `src/codegen/lir/ir.hpp`, `src/codegen/lir/call_args_ops.hpp`, `src/codegen/lir/hir_to_lir/call/target.cpp`, `src/codegen/lir/hir_to_lir/call/args.cpp`, `src/codegen/lir/lir_printer.cpp`, and focused LIR verifier/tests if the new fact carrier requires them.

Proof command:

```sh
cmake --build --preset default && \
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll && \
rg "declare signext i32 @printf|call signext i32 .*@printf|i32 noundef signext 7" build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
```

## Watchouts

- Do not edit `ideas/open/326_rv64_variadic_target_llvm_route_facts.md` unless durable source intent changes.
- Do not weaken unsupported diagnostics or rewrite expectations as progress.
- Do not add `printf`-specific, fixture-shaped, or named-symbol shortcuts.
- Keep AArch64 structured `%struct.__va_list` behavior intact.
- Prefer target-owned LLVM-route/LIR facts before semantic BIR, prepared ABI, or final emission changes.
- The Step 2 rule should be target/profile and ABI driven: apply to direct external variadic integer calls on RV64 generally, not to the spelling `@printf`.
- Current `LirExternDecl` only carries return type text/type ref/name/link id, so declaration attributes need an explicit carrier or a target-owned rendered return type policy.
- Current `LirCallArg` has AArch64 HFA/stack alignment metadata but no integer extension metadata. Adding `signext` by embedding raw text into `.type` would be expedient but risks parser/verifier coupling; prefer a structured fact if practical.
- Step 3 should likely change `llvm_va_list_is_pointer_object`/storage policy for RV64 plus add an explicit RV64 pointer-cursor `va_arg` lowering instead of routing RV64 through `LirVaArgOp` if the desired IR must expose cursor load, 8-byte GEP, cursor store, and scalar load.

## Proof

Ran the delegated Step 1 proof successfully; output is preserved in `test_after.log`:

```sh
cmake --build --preset default
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/direct_variadic_printf.c -o build/variadic_abi_triage/direct_variadic_printf.c4c.rv64.ll
./build/c4cll --codegen llvm --target riscv64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.rv64.ll
./build/c4cll --codegen llvm --target aarch64-linux-gnu build/variadic_abi_triage/stdarg_scalar_int.c -o build/variadic_abi_triage/stdarg_scalar_int.c4c.aarch64.ll
```

The proof is sufficient for Step 1 fact-location only; it intentionally records current incorrect RV64 shapes rather than proving an implementation repair.
