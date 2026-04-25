Status: Active
Source Idea Path: ideas/open/111_lir_struct_decl_printer_authority_switch.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Legacy Shadow And Backcompat Behavior

# Current Packet

## Just Finished

Step 3 - Prove Legacy Shadow And Backcompat Behavior is complete.

`tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp` now proves the
hand-built module keeps matching `struct_decls` and legacy `type_decls`, that
the printer emits the matching struct declaration, and that `verify_module()`
rejects a general mismatch between the structured declaration render and the
legacy `type_decls` line.

The existing extern declaration mirror rejection remains covered, and
`type_decls` remains populated as legacy verifier/backcompat data.

## Suggested Next

Delegate Step 4 to run the focused LLVM parity coverage selected by the
supervisor.

## Watchouts

- Keep `type_decls` populated as legacy proof/backcompat data.
- Leave verifier shadow/parity checks active.
- Do not broaden the switch to global, function, extern, call, backend, BIR,
  MIR, or layout lookup authority.
- Do not rewrite expectations or downgrade tests as proof.
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
