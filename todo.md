Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Extract Dynamic Stack and Entry Formal Helpers

# Current Packet

## Just Finished

Step 1 extracted the dynamic stack helper cluster into
`dispatch_dynamic_stack.cpp/hpp` and the entry formal publication helper
cluster into `dispatch_entry_formals.cpp/hpp`, then wired both new sources into
`src/backend/CMakeLists.txt`. Dispatch call sites now include the new headers
and call the extracted roots without changing dispatch ordering or test
expectations.

## Suggested Next

Execute Step 2 from `plan.md`: use AST-backed queries to identify the diagnostic
helper cluster and extract it into `dispatch_diagnostics.cpp/hpp` if the helper
dependencies remain mechanically separable.

## Watchouts

- AST evidence recorded for Step 1:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees lower_dynamic_stack_helper_call`,
  `function-callers lower_dynamic_stack_helper_call`,
  `function-callees lower_entry_formal_publications`, and
  `function-callers lower_entry_formal_publications`.
- Dynamic-stack extraction kept `make_bir_machine_instruction` in
  `dispatch.cpp` for existing branch-compare callers and copied a private local
  equivalent into `dispatch_dynamic_stack.cpp` for the extracted dynamic-stack
  helpers.
- Entry-formal extraction copied only the small private lookup/alignment helpers
  needed by the moved publication cluster; no grouping adjustment or source idea
  change was needed.

## Proof

Fresh Step 1 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```
