Status: Active
Source Idea Path: ideas/open/188_lir_bir_freeze_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Build The Freeze Ledger

# Current Packet

## Just Finished

Completed `plan.md` Step 1 by building the LIR/BIR freeze ledger from closed ideas 183, 184, 185, 186, 187, 189, 190, 191, and 194.

Freeze ledger:

- Idea 183, backend freeze authority audit: classified the relevant LIR/BIR identity domains before backend restart. Covered direct-call signatures, direct symbol identity, global/type declaration compatibility tables, aggregate layout facts, memory provenance global handles, and prealloc route-local names. Retained strings were classified as display/output, diagnostics, route-local handles, raw/no-metadata compatibility, or semantic authority requiring follow-up.
- Idea 184, direct-call signature metadata: repaired the generated direct-call signature domain. Metadata-rich direct calls now use structured callee/function signature facts for return type, parameter types, variadic state, byval, sret, and aggregate layout. Rendered signature text remains acceptable only as compatibility/display spelling for explicit no-metadata paths.
- Idea 185, global/type declaration compatibility fence: repaired/fenced the selected generated aggregate-global type/layout domain. Structured layout identity, `LirTypeRef`, and `StructNameId` own the selected generated path; string-keyed `GlobalTypes`, `TypeDeclMap`, and layout compatibility tables remain acceptable as raw/no-id compatibility bridges and final/display spelling boundaries. Pointer initializer paths carrying `initializer_function_link_name_ids` were called out as adjacent follow-up territory, not part of this selected fence.
- Idea 186, direct symbol identity validation: repaired the BIR direct symbol identity domain for direct calls, globals, and pointer initializers. Metadata-rich direct symbol references validate through `LinkNameId`; raw callee/global strings remain acceptable for display, diagnostics, runtime/intrinsic placeholder calls, and explicit raw/no-id compatibility.
- Idea 187, memory provenance global handles: repaired the addressed-global pointer provenance domain. `AddressedGlobalPointerSlots` and `AddressedGlobalPointerValueSlots` are keyed by `LinkNameId`; local slot, local SSA, temporary, and route-local memory names remain acceptable string handles because they are not global symbol authority. Raw-import compatibility string paths remain fenced.
- Idea 189, no-prototype/variadic direct-call compatibility: repaired the structured direct-call signature compatibility domain for old-style, no-prototype, and variadic C calls. No-prototype flexibility and variadic fixed/tail argument handling are represented intentionally in structured metadata; rendered function names or printed signatures are not semantic authority for generated calls.
- Idea 190, call argument structured payload: repaired the LIR call argument domain. Generated metadata-rich `LirCallOp` sites carry structured argument facts for operand, type text mirror, byval type-ref facts, and related call argument metadata. `callee_type_suffix` and `args_str` remain acceptable only as final spelling and raw/no-metadata compatibility payloads.
- Idea 191, function signature byval metadata: repaired the BIR function-signature byval domain. `LirSignatureParam::is_byval` is the structured explicit-byval marker for generated signatures, and BIR aggregate parameter collection consumes structured metadata instead of parsing `signature_text`. `signature_text` remains acceptable as final header spelling and legacy no-metadata fixture compatibility.
- Idea 194, global memory provenance `LinkNameId` expansion: repaired an additional global memory/provenance route through dynamic global scalar-array materialization. Structured `LinkNameId` provenance is carried through dynamic global aggregate/scalar array access into `bir::LoadGlobalInst::global_name_id`; raw spelling lookup remains acceptable only at the explicit `kInvalidLinkName` compatibility boundary.

Retained string boundaries accepted by this ledger:

- Printer, dump, diagnostic, ABI, object/output, and final spelling strings are retained as presentation or emitted-spelling boundaries, not semantic metadata authority.
- Raw hand-authored LIR/BIR and no-metadata fixture compatibility remains retained where paths are explicitly fenced by absent or invalid structured ids.
- Route-local names for local SSA values, slots, temporaries, block labels, and prealloc bookkeeping remain retained because they are local handles rather than global/function/type identity.
- Runtime/intrinsic placeholder calls remain invalid-id compatibility paths rather than user/extern symbol identity.
- Generated metadata-rich direct-call, call-argument, byval-signature, direct-symbol, selected global/type/layout, addressed-global pointer, and dynamic global scalar-array routes are now structured-id/fact authoritative and fail closed on stale or missing metadata according to their closed idea notes.

Uncertain generated-path string-authority surfaces for Step 2 audit:

- Pointer initializer address/global resolution remains the main named adjacent surface. Idea 185 explicitly left pointer initializer paths carrying `initializer_function_link_name_ids` as follow-up territory, while idea 186 reports pointer initializer structured symbol references have fail-closed coverage. Step 2 should verify the current pointer-initializer generated path is still `LinkNameId`-authoritative and does not reintroduce spelling-first `global_types.find(global_name)` or `GlobalAddress` semantics.
- Remaining memory/provenance `global_types.find(global_name)` and `GlobalAddress` spelling flows outside the idea 187 addressed-global pointer path and the idea 194 dynamic global scalar-array route should be audited as route-local/no-id compatibility or named as a narrow follow-up if any metadata-rich path is still spelling-first.
- Prealloc route-local names were classified by idea 183 but not converted by the dependency wave. Step 2 should verify they are only route-local handles and not a generated-path semantic function/global/type authority.

## Suggested Next

Execute Step 2 by auditing the ledger's uncertain generated-path surfaces against current code, especially pointer initializer global resolution, remaining memory/provenance global-name lookup flows, and prealloc route-local names.

## Watchouts

- Do not start backend restart work inside this gate.
- Do not patch implementation as part of ledger collection.
- Do not treat printer/output, diagnostics, raw/no-id fixtures, route-local SSA/slot/block labels, or ABI/final spelling as defects unless they are used as metadata-rich semantic authority.
- If a remaining generated-path string-authority blocker is found during Step 2, record it precisely for a narrow follow-up idea instead of broadening the freeze gate.

## Proof

Ledger-only packet; no command was required or run beyond read-only inspection of the closed dependency idea files. Existing `test_after.log` was left untouched.
