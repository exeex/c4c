Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-Write Extern Declaration Mirrors

# Current Packet

## Just Finished

Implemented `plan.md` Step 4 extern declaration mirror dual-write.
`LirModule::ExternDeclInfo` now preserves a structured return type mirror during
record/dedup, `finalize_module()` carries that mirror into `LirExternDecl`, and
known struct return declarations receive matching `StructNameId` identity while
`LirExternDecl::return_type_str` remains the printed declaration text. The
verifier now checks extern return mirror text parity against `return_type_str`
and rejects known struct extern returns that lack the matching `StructNameId`.
Focused frontend/LIR coverage records a `%struct.Pair` extern declaration,
observes its structured mirror, verifies missing-`StructNameId` rejection, and
confirms printed extern declaration output remains `declare %struct.Pair ...`.

## Suggested Next

Next bounded implementation packet: implement Step 5 call return and argument
type mirrors where structured identity is already available. Keep extern
declaration mirrors as the completed input surface and avoid widening into ABI
classification or printer authority changes.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function
signature parameter mirrors intentionally shadow the emitted signature fragment;
for ABI-expanded aggregate parameters, the current `LirTypeRef` field shape
cannot represent both the emitted byval fragment and underlying aggregate
identity without violating `StructNameId`/text parity, so those parameter
mirrors are raw/no-`StructNameId` for now. Extern declaration mirrors only attach
structured identity when `return_type_str` resolves to an existing
`LirStructDecl`; raw non-struct and unknown return text remains unstructured.
Call mirror paths are still untouched.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 7/7 passing
focused tests: `frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`frontend_lir_extern_decl_type_ref`, `positive_split_llvm_pragma_exec`, and the
three `abi_` tests.
