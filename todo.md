Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Diagnostics Helpers

# Current Packet

## Just Finished

Step 2 extracted the dispatch diagnostic construction/formatting helper cluster
into `dispatch_diagnostics.cpp/hpp` and wired the new source into
`src/backend/CMakeLists.txt`. The moved cluster keeps the same diagnostic record
fields and emitted message text for block diagnostics, unsupported terminators,
unsupported instruction-family diagnostics, and call diagnostics.

## Suggested Next

Execute Step 3 from `plan.md`: use AST-backed queries to identify the value
lookup and producer lookup helper cluster, then extract mechanically separable
lookup helpers into `dispatch_lookup.cpp/hpp`.

## Watchouts

- AST evidence recorded for Step 2:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees/function-callers append_block_diagnostic`,
  `function-callees/function-callers unsupported_terminator_message`,
  `function-callees/function-callers append_unsupported_instruction_diagnostic`,
  `function-callees/function-callers append_call_diagnostic`, and
  `function-callees/function-callers classify_instruction`.
- `classify_instruction` moved as a private helper in
  `dispatch_diagnostics.cpp` because AST callers showed it is only used by
  `append_unsupported_instruction_diagnostic`.
- `dispatch_dynamic_stack.cpp` still has its own private `append_call_diagnostic`
  copy from Step 1; Step 2 did not broaden ownership into that file.

## Proof

Fresh Step 2 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```
