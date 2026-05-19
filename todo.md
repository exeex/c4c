Status: Active
Source Idea Path: ideas/open/308_aarch64_extern_data_symbol_pic_address_formation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Symbol Binding Address Lowering

# Current Packet

## Just Finished

Step 1 located the direct `stdout` address path for `00189.c` without editing
implementation files. The generated bad assembly is
`build/c_testsuite_aarch64_backend/src/00189.c.s` lines 37-39:
`adrp x10, stdout` / `add x10, x10, :lo12:stdout` / `ldr x9, [x10]`, matching
the `test_before.log` linker failure for `R_AARCH64_ADR_PREL_PG_HI21` against
externally binding `stdout@@GLIBC_2.17`.

The semantic path is:

- builtin `stdio.h` declares `extern FILE *stdout;`
  (`src/frontend/preprocessor/preprocessor.cpp`);
- HIR-to-LIR records extern globals as `LirGlobal::is_extern_decl = true` and
  static/internal globals as `LirGlobal::is_internal = true`
  (`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`);
- BIR keeps the metadata as `bir::Global::is_extern`, `is_thread_local`, and
  `address_materialization_policy` (`src/backend/bir/bir.hpp`);
- `lower_global_address_materialization_policy` in
  `src/backend/bir/lir_to_bir/globals.cpp` currently returns `Direct` for
  target relocation model `Static` before checking external binding, returns
  `Direct` for `global.is_internal`, and otherwise leaves the policy
  `Unspecified`;
- prepared addressing in
  `append_direct_global_address_materialization`
  (`src/backend/prealloc/stack_layout/coordinator.cpp`) maps
  `GotRequired` to `PreparedAddressMaterializationKind::GotGlobal`, maps
  direct/TLS cases to `DirectGlobal`/`TlsGlobal`, and only fails closed on
  `Unspecified` when the target relocation model is non-static;
- AArch64 selection maps `DirectGlobal` to `DirectPageLow12` and `GotGlobal`
  to `GotPageLow12` in `selected_address_materialization_kind`
  (`src/backend/mir/aarch64/codegen/globals.cpp`);
- the final direct emitter is
  `print_address_materialization_instruction`
  (`src/backend/mir/aarch64/codegen/globals.cpp`), whose
  `DirectPageLow12` path prints `adrp <reg>, <sym>` plus
  `add <reg>, <reg>, :lo12:<sym>`, while `GotPageLow12` already prints
  `adrp <reg>, :got:<sym>` plus
  `ldr <reg>, [<reg>, :got_lo12:<sym>]`.

Existing coverage found:

- `backend_prepare_stack_layout_test.cpp` already proves explicit
  `GotRequired` becomes `PreparedAddressMaterializationKind::GotGlobal`, direct
  local/global materialization remains `DirectGlobal`, TLS remains separate, and
  PIC + `Unspecified` policy fails closed with a diagnostic.
- `backend_aarch64_prepared_memory_operand_records_test.cpp` proves prepared
  direct/GOT/TLS carriers preserve policy and identity into AArch64 records and
  fail closed on missing or mismatched GOT policy.
- `backend_aarch64_machine_printer_test.cpp` proves the direct
  `adrp`/`:lo12:` printer path and the existing GOT printer path.
- Gap: no current test appears to prove that a LIR/BIR extern data global such
  as `stdout` is assigned `GotRequired` while local/internal compiler-owned
  data remains direct, especially when the driver/test harness does not pass
  `-fPIC` but the system linker defaults to PIE.

## Suggested Next

Execute Step 2 at the semantic policy point, not in the printer: add or adjust
the predicate feeding `bir::Global::address_materialization_policy`, most
likely `lower_global_address_materialization_policy`, so externally binding
data globals (`LirGlobal::is_extern_decl` / `bir::Global::is_extern`, not
`is_internal`, not TLS) get `GotRequired` for AArch64 dynamic/PIE-safe address
formation while local/internal/compiler-owned globals keep `Direct`.

## Watchouts

- Do not implement filename-specific or `stdout`-specific shortcuts.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, runner behavior, timeout policy, or proof logs to claim
  progress.
- Preserve legal direct local/internal symbol address formation.
- Do not route string constants, labels, TLS, or same-module/internal function
  symbols through the external-data GOT rule.
- The AArch64 c-testsuite runner invokes `c4cll` without `-fPIC`/`-fPIE`, while
  the host `clang --target=aarch64-unknown-linux-gnu` link still rejected the
  direct relocation as PIE/shared-object unsafe. Step 2 should account for that
  semantic mismatch explicitly instead of relying only on the current
  `TargetRelocationModel::Pic` gate.
- Keep runtime, timeout, machine-printer/prepared-node, and `lir_to_bir`
  admission buckets parked under umbrella idea 295.

## Proof

No tests were rerun per packet instruction. Proof artifacts inspected:
`test_before.log`, `build/c_testsuite_aarch64_backend/src/00189.c.s`, and
read-only code/test inspection. No `test_after.log` was generated or modified
because this was a localization-only packet with an explicit no-rerun proof
contract.
