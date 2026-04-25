Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate Parity And Broader Stability

# Current Packet

## Just Finished

Completed `plan.md` Step 6 / Step 3 verifier regression fix for function
signature parameter mirrors. `verify_function_signature_type_ref_shadows()` now
keeps commas inside aggregate literal braces and brackets nested while still
respecting existing parenthesized fragments, so parameters such as
`{ double, double }` shadow the emitted signature text as one parameter instead
of being split at the inner comma. Added focused frontend/LIR verifier coverage
for aggregate literal parameter text paired with an existing `byval(...)`
parameter.

## Suggested Next

Next bounded packet: supervisor review/commit of this Step 6 verifier parsing
slice, or broader Step 6 parity validation if policy requires another pass.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function
signature parameter mirrors intentionally shadow the emitted signature fragment,
including raw aggregate LLVM text like `{ double, double }`; this packet only
changed verifier parsing and did not alter lowering or printer authority logic.
Extern declaration mirrors only attach structured identity when
`return_type_str` resolves to an existing `LirStructDecl`; raw non-struct and
unknown return text remains unstructured.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_.*|positive_split_llvm_.*|abi_.*|positive_sema_ok_call_variadic_aggregate_runtime_c|llvm_gcc_c_torture_src_(complex_1|complex_2|complex_5|complex_6|complex_7|20010605_2|20070614_1|pr38969|pr42248)_c)$'; } > test_after.log 2>&1`

Result: passed 18/18. `test_after.log` contains a successful build and all
selected `frontend_lir_*`, `positive_split_llvm_*`, `abi_*`,
`positive_sema_ok_call_variadic_aggregate_runtime_c`, and the 9 named
`llvm_gcc_c_torture` regression cases passing.
