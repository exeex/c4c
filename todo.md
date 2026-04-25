Status: Active
Source Idea Path: ideas/open/111_lir_struct_decl_printer_authority_switch.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Switch Struct Declaration Printing To struct_decls

# Current Packet

## Just Finished

Step 2 - Switch Struct Declaration Printing To struct_decls is complete.

`src/codegen/lir/lir_printer.cpp` now emits LLVM struct/type declaration lines
by iterating `mod.struct_decls` in vector order and rendering each declaration
with `render_struct_decl_llvm(mod, decl)`.

The legacy `type_decls` population path was not changed, and
`print_llvm()` still calls `verify_module(mod)` before emission, so the
existing verifier shadow/parity checks remain active as backcompat proof.

## Suggested Next

Delegate Step 3 to prove that legacy `type_decls` shadow/backcompat behavior
still rejects mismatches after the printer authority switch.

## Watchouts

- Keep `type_decls` populated as legacy proof/backcompat data.
- Leave verifier shadow/parity checks active.
- Do not broaden the switch to global, function, extern, call, backend, BIR,
  MIR, or layout lookup authority.
- Do not rewrite expectations or downgrade tests as proof.
- Step 3 should exercise general verifier parity behavior, not a named-case
  shortcut or expectation rewrite.
- Hand-built tests that populate `type_decls` without `struct_decls` are no
  longer a source of emitted LLVM struct declaration lines; keep any
  compatibility decision explicit and tied to verifier/backcompat proof.

## Proof

Supervisor-selected proof passed, and the full output is preserved in
`test_after.log`.

Command:
`{ cmake --build build --target frontend_lir_global_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_extern_decl_type_ref_test frontend_lir_call_type_ref_test && ctest --test-dir build -R 'frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)' --output-on-failure; } > test_after.log 2>&1`

Result:
- Build completed for all four delegated frontend LIR test targets.
- CTest passed 4/4:
  `frontend_lir_global_type_ref`,
  `frontend_lir_function_signature_type_ref`,
  `frontend_lir_extern_decl_type_ref`, and
  `frontend_lir_call_type_ref`.
