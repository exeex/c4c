Status: Active
Source Idea Path: ideas/open/108_lir_struct_name_id_for_globals_functions_and_externs.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Dual-Write Function Signature Mirrors

# Current Packet

## Just Finished

Strengthened `plan.md` Step 2 globals coverage with a focused
`frontend_lir_global_type_ref` CTest target. The test lowers C source through
parser/sema/HIR-to-LIR, observes both a top-level struct global and a top-level
union global, and asserts that each `LirGlobal::llvm_type_ref` is present,
textually matches `llvm_type`, carries a `StructNameId`, and resolves that ID
through `LirModule::struct_names`. It also checks printed LLVM global lines
still use the legacy `llvm_type` text.

## Suggested Next

Next bounded implementation packet: implement the function signature mirror
surface from the Step 1 audit. Keep `signature_text` as printer authority and
avoid extern declaration or call-site mirror changes in the same packet unless
the supervisor explicitly widens the slice.

## Watchouts

`lir_global_type_ref()` intentionally rejects flexible-array literal globals
because their literal aggregate text does not match the canonical named struct
type. Do not relax that guard without a separate semantic design. Function,
extern, and call mirror paths are still untouched.

## Proof

Ran the supervisor-selected proof exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(positive_split_llvm_|abi_)'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains a successful build and 4/4 passing
focused tests (`positive_split_llvm_pragma_exec` and three `abi_` tests).

Also ran the newly added focused coverage because it is outside that regex:

`cmake --build --preset default --target frontend_lir_global_type_ref_test && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_global_type_ref$'`

Result: passed. The supervisor then normalized `test_after.log` back to the
same 4-test scope as `test_before.log` and ran the regression guard with
`--allow-non-decreasing-passed`; before and after both reported 4/4 passing
with no new failures.
