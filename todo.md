Status: Active
Source Idea Path: ideas/open/384_aarch64_dispatch_mechanical_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Extract Branch and Compare Fusion Helpers

# Current Packet

## Just Finished

Step 4 extracted the branch and compare fusion helper cluster into
`dispatch_branch_fusion.cpp/hpp` and wired the new source into
`src/backend/CMakeLists.txt`. The moved cluster preserves the existing fused
branch ordering, compare opcode suffix selection, compare materialization, and
emitted-condition branch fallback by routing remaining dispatch-private services
through a narrow hook table.

## Suggested Next

Execute Step 5 from `plan.md`: use AST-backed queries to identify the next
mechanically separable publication helper cluster before moving publication
logic out of `dispatch.cpp`.

## Watchouts

- AST evidence recorded for Step 4:
  `c4c-clang-tool-ccdb function-signatures .../dispatch.cpp build/compile_commands.json`,
  `function-callees/function-callers lower_fused_compare_branch_from_emitted_cast`,
  `function-callees/function-callers lower_materialized_compare_condition_branch`,
  `function-callees/function-callers lower_current_block_entry_fused_compare_branch`,
  `function-callees/function-callers lower_constant_rhs_fused_compare_branch`,
  `function-callees/function-callers lower_conditional_branch_from_emitted_condition`,
  `function-callees/function-callers is_fused_compare_branch_support_instruction`,
  and `function-callees/function-callers lower_stack_home_fused_compare_branch`.
- The extracted helpers depend on value publication, producer lookup, stack-home
  emission, and entry-publication lookup. Those services remain owned by
  `dispatch.cpp` and are passed through `DispatchBranchFusionHooks` rather than
  broadening Step 4 into publication or storage-plan extraction.
- `branch_condition_suffix` and `is_cmp_immediate_encodable` are now public in
  `dispatch_branch_fusion.hpp` because existing compare/select materialization
  code in `dispatch.cpp` still uses the same opcode and immediate contracts.

## Proof

Fresh Step 4 proof passed and is preserved in `test_after.log`:

```bash
(cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test -j10 && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00204_c)$' --output-on-failure) > test_after.log 2>&1
```
