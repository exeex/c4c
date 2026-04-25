Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Dual-Write Function Signature Mirrors

# Current Packet

## Just Finished

Repaired `plan.md` Step 3 function signature mirror dual-write for ABI-expanded
aggregate parameters. `LirFunction` still carries optional structured return
type mirrors and emitted parameter type mirrors, but HIR-to-LIR now only attaches
`StructNameId` identity when the mirror text is the canonical aggregate type
text. Focused frontend/LIR coverage now proves normal struct return and
parameter mirrors carry `StructNameId`, while a large AMD64 SysV byval aggregate
parameter keeps its emitted `ptr byval(...) align ...` mirror text raw and
without invalid struct-name identity. Printed LLVM signatures remain unchanged.

## Suggested Next

Next bounded implementation packet: implement Step 4 extern declaration return
type mirrors while preserving `LirExternDecl::return_type_str` as the printer
authority. Keep function signature and call-site mirror surfaces out of that
slice unless the supervisor explicitly widens it.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function
signature parameter mirrors intentionally shadow the emitted signature fragment;
for ABI-expanded aggregate parameters, the current `LirTypeRef` field shape
cannot represent both the emitted byval fragment and underlying aggregate
identity without violating `StructNameId`/text parity, so those parameter
mirrors are raw/no-`StructNameId` for now. Extern declaration and call mirror
paths are still untouched.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 6/6 passing
focused tests: `frontend_lir_global_type_ref`,
`frontend_lir_function_signature_type_ref`,
`positive_split_llvm_pragma_exec`, and the three `abi_` tests.
