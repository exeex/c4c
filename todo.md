Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add or require the selected parameter family-ref carrier

# Current Packet

## Just Finished

Completed plan.md Step 1 inventory for extern fixed parameter type facts.
`record_extern_decl` and `ExternDeclInfo` currently own extern identity,
return text/ref, return extension, and an optional `function_signature_ref`;
fixed parameter facts live in `LirFunctionSignatureStoreEntry` as
`fixed_param_type_refs`, `fixed_param_is_byval`, `is_variadic`, and
`has_void_param_list`. The signature store is the selected producer-owned
structured carrier for the first Step 2 slice; no minimal direct
`LirExternDecl` parameter field is justified yet.

## Suggested Next

Execute plan.md Step 2 target: require/publish a link-backed extern fixed
aggregate byval parameter family ref through the nominal signature store.
Bound the slice to direct extern calls whose `callee_signature_ref` resolves to
`extern_decl_link_name_map[LinkNameId].function_signature_ref`, with one fixed
parameter whose `fixed_param_type_refs[index]` carries the matching
`StructNameId` and whose `fixed_param_is_byval[index]` is true.

Accepted authority inputs for this target are the extern LinkNameId, the
signature-store ref on `ExternDeclInfo`/`LirExternDecl`, the store entry's
fixed parameter type refs and byval bits, and matching structured declarations
plus aggregate-store facts for the selected `StructNameId`. Rejected authority
inputs/non-goals are `return_type`/`return_type_str`, symbol spelling alone,
varargs metadata, final declaration text, initializer payload text,
collector-only receiver observations, Raw-BIR import work, non-type string
routing, and recovery from stale `callee_type_suffix` or `args_str`.

Required proof needs for the next code packet:
positive link-backed extern byval parameter with matching `StructNameId`;
malformed store entry with missing/size-mismatched byval facts; stale-text case
where rendered call/declaration text names a different parameter family but the
store wins or fails closed; missing-carrier case with no valid extern
`function_signature_ref`; wrong-family case with mismatched parameter
`StructNameId`; compatibility case proving nonaggregate extern fixed
parameters and existing extern return/initializer output remain supported.

Exact proof command for the next code packet:
`cmake --build build && ctest --test-dir build -R backend_lir_to_bir_notes_test --output-on-failure`.

## Watchouts

Current carriers are split: `record_extern_decl` does not accept parameter
facts; `register_extern_function_signature` can attach a nominal
`function_signature_ref` to an existing extern map entry; `LirExternDecl`
snapshots only that ref for printer/BIR adapter parity; the printer can render
function/call parameter lists from the store but extern declarations still only
render return/name; verifier call checks already require a callee signature ref
to match the resolved module function or extern declaration. The missing
evidence is a focused externally declared aggregate-byval parameter test proving
the extern store ref, not final text, is the semantic family carrier.

Keep fixed parameter facts separate from return facts, symbol identity, varargs
policy, final declaration text, initializer payload text, collector-only
receiver work, Raw-BIR import work, and non-type string routing. Do not add a
direct `LirExternDecl` parameter API unless the selected signature-store packet
fails because an actual consumer needs declaration-local parameter ownership
that the nominal store cannot provide.

## Proof

Inventory-only packet. Ran `git diff --check`; no build/test proof was required
for Step 1 and no `test_after.log` was produced.
