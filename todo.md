Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate Parity And Broader Stability

# Current Packet

## Just Finished

Completed `plan.md` Step 5 call return and argument mirror dual-write.
`LirCallOp::return_type` now receives structured `LirTypeRef` identity for known
struct/union call returns, and `OwnedLirTypedCallArg` can carry a structured
argument mirror through `format_lir_call_fields()` into `LirCallOp::arg_type_refs`
when the emitted argument fragment is the canonical aggregate type. The verifier
checks call return/argument mirror parity against existing call text and rejects
known struct call mirrors that lose their matching `StructNameId`. Added focused
frontend/LIR coverage for a direct `%struct.Pair` call return and argument,
including formatting/printing stability and missing-`StructNameId` rejection.

## Suggested Next

Next bounded packet: execute `plan.md` Step 6 validation as directed by the
supervisor, using the completed global, function signature, extern declaration,
and call mirror surfaces as the acceptance target.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function
signature parameter mirrors intentionally shadow the emitted signature fragment.
For ABI-expanded aggregate call arguments, the current `LirTypeRef` field shape
cannot represent both the emitted byval/packed fragment and underlying
aggregate identity without violating text parity, so those arguments remain
raw/no-`StructNameId` and calls with no structured argument mirror keep
`arg_type_refs` empty. Extern declaration mirrors only attach structured
identity when `return_type_str` resolves to an existing `LirStructDecl`; raw
non-struct and unknown return text remains unstructured.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 8/8 passing
focused tests: `frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`frontend_lir_extern_decl_type_ref`, `frontend_lir_call_type_ref`,
`positive_split_llvm_pragma_exec`, and the three `abi_` tests.
