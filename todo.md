Status: Active
Source Idea Path: ideas/open/111_lir_struct_decl_printer_authority_switch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Declaration Emission And Parity Paths

# Current Packet

## Just Finished

Step 1 - Inspect Declaration Emission And Parity Paths is complete.

Precise printer edit point:
- `src/codegen/lir/lir_printer.cpp:529-530` is the struct/type declaration
  emission loop inside `print_llvm()`: it currently prints each
  `mod.type_decls` line directly after target datalayout/triple emission and
  before string pool/global emission.
- `src/codegen/lir/lir_printer.cpp:488-516` already defines
  `render_struct_decl_llvm(mod, decl)`, the structured renderer used by the
  verifier. Step 2 should switch only the declaration loop to render
  `mod.struct_decls` through this helper.

Declaration order preservation:
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:372-488` builds legacy
  `type_decls` and structured `struct_decls` in the same sequence: optional
  target `__va_list_tag_` declaration first, then `mod.struct_def_order`.
- Every legacy `decls.push_back(...)` in `build_type_decls()` is paired with
  `record_struct_decl(...)` for the same declaration path, and
  `LirModule::record_struct_decl()` appends new names at
  `src/codegen/lir/ir.hpp:694-708`.
- Re-recording an existing `StructNameId` replaces in place rather than
  reordering, so Step 2 must iterate `mod.struct_decls` in vector order and
  avoid sorting or map iteration.

Verifier checks that must remain:
- `verify_module()` calls `verify_struct_decl_shadows()` first at
  `src/codegen/lir/verify.cpp:725-727`; `print_llvm()` calls `verify_module()`
  before emission at `src/codegen/lir/lir_printer.cpp:518-519`.
- `verify_struct_decl_shadows()` must keep checking that each structured
  declaration has a resolvable name, has a matching legacy `type_decls` line,
  has valid field `LirTypeRef`s, and renders exactly equal to the legacy line
  via `render_struct_decl_llvm()` (`src/codegen/lir/verify.cpp:409-442`).
- Adjacent parity/shadow checks in `verify_module()` for structured layout
  observations, extern returns, globals, and function signatures should remain
  unchanged.

Where `type_decls` remains populated:
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:1076` still assigns
  `module.type_decls = build_type_decls(hir_mod, &module)`.
- Backend/BIR/MIR aggregate consumers still read `module.type_decls`; this
  packet found no Step 2 reason to touch those paths.

## Suggested Next

Delegate Step 2 to replace only the `print_llvm()` declaration emission loop
with ordered iteration over `mod.struct_decls`, rendering each entry with
`render_struct_decl_llvm(mod, decl)`, while leaving `type_decls` population and
all verifier checks intact.

## Watchouts

- Keep `type_decls` populated as legacy proof/backcompat data.
- Leave verifier shadow/parity checks active.
- Do not broaden the switch to global, function, extern, call, backend, BIR,
  MIR, or layout lookup authority.
- Do not rewrite expectations or downgrade tests as proof.
- Ordering risk: `struct_decls` is order-safe only if Step 2 iterates the
  vector, not `struct_decl_index`.
- Parity risk: empty nonzero structs/unions and target va-list declarations
  rely on `build_type_decls()` continuing to populate both mirrors in the same
  branch.
- Hand-built tests may populate `type_decls` without `struct_decls`; Step 2
  should decide whether bare legacy-only modules are intentionally no longer
  declaration-printable or need a compatibility fallback before changing code.

## Proof

Audit-only packet; no build or ctest was required, and no `test_after.log` was
created.

Read-only commands used:
- `git status --short`
- `sed -n` reads of `AGENTS.md`, `plan.md`, `todo.md`, and
  `ideas/open/111_lir_struct_decl_printer_authority_switch.md`
- `rg -n "type_decls|struct_decls|render_struct_decl_llvm|verify_struct_decl_shadows|..." src tests -S`
- `c4c-clang-tool-ccdb find-definition ... render_struct_decl_llvm ...`
- `c4c-clang-tool-ccdb function-callees ... print_llvm ...`
- `c4c-clang-tool-ccdb function-callees ... verify_struct_decl_shadows ...`
- `c4c-clang-tool-ccdb function-callees ... build_type_decls ...`
- focused `nl -ba`/`sed -n` reads of `lir_printer.cpp`, `verify.cpp`,
  `hir_to_lir.cpp`, `ir.hpp`, and `tests/frontend/CMakeLists.txt`

Proposed narrow Step 2 proof command:
`cmake --build build --target frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R 'frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)' --output-on-failure`
