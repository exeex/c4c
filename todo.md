Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate selected parameter verifier/printer consumers

# Current Packet

## Just Finished

Completed plan.md Step 2 for the selected link-backed extern fixed aggregate
byval parameter family-ref carrier. `register_extern_function_signature` now
publishes extern fixed parameter byval bits into the nominal signature store;
direct extern calls whose `callee_signature_ref` resolves through
`extern_decl_link_name_map[LinkNameId].function_signature_ref` now require
matching structured aggregate `fixed_param_type_refs[index]` plus
`fixed_param_is_byval[index] == true`; retained-signature-free lowering can use
the store instead of stale rendered call/declaration text.

## Suggested Next

Execute plan.md Step 3 for the selected extern aggregate byval parameter form:
migrate the selected verifier/printer declaration consumer path to render or
check extern fixed parameter facts from the structured signature-store carrier
while preserving existing declaration output compatibility. Keep the next
packet bounded to the already-selected extern aggregate byval parameter form
and do not widen into returns, initializer text, collector-only receiver work,
Raw-BIR import work, varargs policy, or non-type string routing.

## Watchouts

The corrected proof route for this build is the three-test interface/frontend
subset below; the old `ctest -R backend_lir_to_bir_notes_test` route matches
zero tests and must not be treated as proof. Step 2 deliberately did not add
direct `LirExternDecl` parameter fields; the nominal signature store served as
the selected carrier. Next work should preserve the same scope boundary and
avoid treating final declaration text, `callee_type_suffix`, or `args_str` as
semantic authority.

## Proof

Ran corrected after command:
`{ cmake --build build && ctest --test-dir build -R '^backend_lir_to_bir_interface$|^frontend_lir_extern_decl_type_ref$|^frontend_lir_global_label_address_initializer$' --output-on-failure; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains a successful build and 3/3 passing
tests. Also ran `git diff --check`; passed.
