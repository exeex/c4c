Status: Active
Source Idea Path: ideas/open/09_bir-call-signature-and-abi-family-extraction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Move pure target ABI and return classification helpers

# Current Packet

## Just Finished

Step 2 moved the audited pure ABI helper cluster from `calling.cpp` into
`call_abi.cpp`: `use_float_return_registers`, `use_float_arg_registers`,
`lower_function_return_abi`, `lower_call_arg_abi`, and the two
`lir_to_bir_detail::compute_*_abi` wrappers. `calling.cpp` now calls the
existing `compute_*_abi` detail entry points and no longer defines those helper
bodies.

## Suggested Next

Execute Step 3 by moving function signature and parameter helpers, keeping
typed-call parsing, extern/decl behavior, call lowering, runtime intrinsic
lowering, and failure-note behavior unchanged.

## Watchouts

- This was a behavior-preserving extraction; do not rewrite expectations.
- Do not add new headers.
- Keep `lower_call_inst` and `lower_runtime_intrinsic_inst` in `calling.cpp`
  unless a future packet explicitly owns moving them.
- Step 3 should avoid moving typed-call parsing, extern/decl behavior, or
  failure-note behavior unless specifically delegated.

## Proof

Passed:
`cmake --build build --target c4c_codegen && ctest --test-dir build -R "backend_lir_to_bir_notes" --output-on-failure`.
Proof log: `test_after.log`.
