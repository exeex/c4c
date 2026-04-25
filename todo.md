Status: Active
Source Idea Path: ideas/open/09_bir-call-signature-and-abi-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Move function signature and parameter helpers

# Current Packet

## Just Finished

Step 3 moved the accepted function signature and parameter helper cluster from
`calling.cpp` into `call_abi.cpp`: `is_void_param_sentinel`,
`lower_param_type`, `parse_function_signature_params`,
`lower_return_info_from_type`, `infer_function_return_info`,
`lower_signature_return_info`, and `lower_function_params`. Main typed-call
parsing, extern/decl lowering, call instruction lowering, runtime intrinsic
lowering, and failure-note behavior remain in `calling.cpp`.

## Suggested Next

Execute Step 4 by proving call-family behavior did not change with the
supervisor-selected call/ABI route coverage.

## Watchouts

- This was a behavior-preserving extraction; do not rewrite expectations.
- No new headers were added; declarations remain in `lowering.hpp`.
- Step 4 should validate direct/indirect calls, byval/sret, extern/decl
  functions, and runtime intrinsic routes rather than moving more behavior.

## Proof

Passed:
`cmake --build build --target c4c_codegen && ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`.
Proof log: `test_after.log`.
