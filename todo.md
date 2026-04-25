Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate Parity And Broader Stability

# Current Packet

## Just Finished

Completed `plan.md` Step 6 validation regression fix for the Step 5 call
argument mirror path. `LirCallOp::arg_type_refs` is now emitted only when the
formatted call site can be parsed for argument mirror validation; structured
direct-call `%struct.Pair` arguments still keep their `StructNameId`, while
variadic/ABI-expanded aggregate calls whose fixed-only signature cannot parse
against emitted arguments remain raw with no argument mirrors. Added focused
frontend/LIR coverage for that suppression boundary.

## Suggested Next

Next bounded packet: supervisor review/commit of the Step 6 regression fix, or
the next Step 6 parity pass if broader validation policy requires it.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function
signature parameter mirrors intentionally shadow the emitted signature fragment.
For variadic and ABI-expanded aggregate call arguments, the current
`LirTypeRef` field shape cannot represent both the emitted byval/packed
fragment and underlying aggregate identity without violating text parity, so
calls whose formatted argument list cannot be parsed against the call signature
now keep `arg_type_refs` empty. Extern declaration mirrors only attach
structured identity when `return_type_str` resolves to an existing
`LirStructDecl`; raw non-struct and unknown return text remains unstructured.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_|positive_sema_ok_call_variadic_aggregate_runtime_c)$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and the delegated
regex-selected `positive_sema_ok_call_variadic_aggregate_runtime_c` passing.
Because the literal delegated regex only selects that one test in this checkout,
also ran a supplemental corrected focused subset:
`ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_.*|positive_split_llvm_.*|abi_.*|positive_sema_ok_call_variadic_aggregate_runtime_c)$'`.
Result: passed 9/9, covering the four `frontend_lir_*` tests,
`positive_split_llvm_pragma_exec`, the three `abi_*` tests, and
`positive_sema_ok_call_variadic_aggregate_runtime_c`.
