Status: Active
Source Idea Path: ideas/open/101_bir_prealloc_missing_call_abi_fallback_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Focused ABI Carrier Proof

# Current Packet

## Just Finished

Completed Step 4 by adding focused ABI carrier proof without editing
implementation files.

`backend_prepare_liveness_test.cpp` now lowers an ordinary source-style
`id_i32` call fixture and proves the lowered BIR supplies explicit ABI carriers
before prealloc planning for:

- the formal parameter `Param::abi`
- the call argument `CallInst::arg_abi`
- the named call result `CallInst::result_abi`
- the function return `Function::return_abi`

The same test verifies prepared call plans consume those carriers through the
concrete RISC-V `a0` argument and result ABI register surfaces, while allowing
no-op cases where the value is already in the ABI register.

`backend_prealloc_call_boundary_classification_test.cpp` now names the retained
direct-BIR function-return compatibility path by checking that
`function_return_storage_kind` has no ordinary authority when `Function::return_abi`
is missing, while `direct_bir_function_return_move_repair` remains the explicit
scalar fallback.

## Suggested Next

Ask the plan owner to evaluate whether the runbook is complete and should close
or whether a final review/cleanup packet is needed.

## Watchouts

- This packet did not edit `backend_prealloc_formal_publications_test.cpp` or
  `backend_prepare_frame_stack_call_contract_test.cpp`; their delegated subset
  coverage stayed green.
- The direct-BIR call-argument and call-result repairs remain covered through
  Step 3 implementation names and existing plan behavior; this packet added the
  practical exported-helper assertion for the retained function-return fallback.

## Proof

Delegated proof command:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R 'backend_(prepare_liveness|prealloc_call_boundary_classification|prealloc_formal_publications|prepare_frame_stack_call_contract)' >> test_after.log 2>&1
```

Result: passed. `test_after.log` contains the delegated build plus CTest output.
CTest passed 4/4:

- `backend_prepare_liveness`
- `backend_prealloc_call_boundary_classification`
- `backend_prealloc_formal_publications`
- `backend_prepare_frame_stack_call_contract`
