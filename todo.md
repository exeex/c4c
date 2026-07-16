Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected parameter legacy mirrors only with parity

# Current Packet

## Just Finished

Completed plan.md Step 3 for the selected link-backed extern fixed aggregate
byval parameter form. The extern declaration printer now renders the selected
fixed aggregate byval parameter list from `LirExternDecl.function_signature_ref`
and the nominal `LirFunctionSignatureStoreEntry` carrier when present, while
legacy extern declarations without the selected structured carrier keep the
existing `(...)` compatibility output.

## Suggested Next

Execute plan.md Step 4 for the selected extern aggregate byval parameter form:
retire or demote only selected parameter text mirrors if all named verifier,
printer, and backend consumers have migrated. If final declaration text,
`callee_type_suffix`, `args_str`, or retained signature text remains required
as compatibility/output text for unselected forms, record a bounded no-code
retirement conclusion instead of deleting it.

## Watchouts

The corrected proof route for this build is the three-test interface/frontend
subset below; the old `ctest -R backend_lir_to_bir_notes_test` route matches
zero tests and must not be treated as proof. Step 3 deliberately did not add
direct `LirExternDecl` parameter fields. Keep Step 4 inside the same selected
extern aggregate byval parameter form and do not widen into return facts,
initializer text, collector-only receiver work, Raw-BIR import work, varargs
policy, nonaggregate declaration rendering, or non-type string routing.

## Proof

Ran corrected after command:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_to_bir_interface$|^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build and 3/3 passing
tests. Also ran `git diff --check`; passed.
