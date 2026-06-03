Status: Active
Source Idea Path: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Broader Validation And Close Readiness

# Current Packet

## Just Finished

Completed Step 5 by running the delegated broader backend validation and
recording close-readiness for the missing call ABI fallback boundary.

Fallback status summary:

- Removed silent ordinary type-only ABI authority from call argument, call
  result, and function return planning paths; ordinary lowered calls now rely on
  explicit BIR ABI carriers.
- Constrained retained direct-BIR/bootstrap compatibility paths by naming them
  as explicit repairs instead of leaving them as implicit fallback authority.
- Retained target placement and no-op move decisions as prealloc authority after
  ABI carriers exist.

Named compatibility repairs now documented by the implementation and tests:

- `direct_bir_call_arg_abi_repair`
- `direct_bir_call_result_abi_repair`
- `direct_bir_function_return_abi_repair`
- `direct_bir_call_arg_bank_display`
- `direct_bir_call_result_bank_display`
- `direct_bir_function_return_move_repair`

Ordinary carrier proof coverage is in place for `Param::abi`,
`CallInst::arg_abi`, `CallInst::result_abi`, and `Function::return_abi`, with
prepared call-plan assertions covering concrete ABI register consumption.

## Suggested Next

Ask the plan owner to evaluate closure for
`ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md`.

## Watchouts

- This Step 5 packet made no implementation or test edits; it only updated
  `todo.md` and refreshed `test_after.log`.
- The retained direct-BIR repairs are compatibility paths, not ordinary lowered
  source-code ABI authority.
- Closure should preserve the distinction between carrier publication by BIR
  and target placement decisions by prealloc.

## Proof

Delegated proof command:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R 'backend_(prepare_liveness|prealloc_call_boundary_classification|prealloc_formal_publications|prepare_frame_stack_call_contract|aarch64_call_boundary_owner|aarch64_return_lowering|call_boundary_effect_plan|x86_call_boundary_effect_ordering)' >> test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated build plus CTest output.
CTest passed 8/8:

- `backend_prepare_liveness`
- `backend_prealloc_call_boundary_classification`
- `backend_prealloc_formal_publications`
- `backend_prepare_frame_stack_call_contract`
- `backend_aarch64_call_boundary_owner`
- `backend_aarch64_return_lowering`
- `backend_call_boundary_effect_plan`
- `backend_x86_call_boundary_effect_ordering`
