Status: Active
Source Idea Path: ideas/open/188_lir_bir_freeze_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Build The Freeze Closure Ledger

# Current Packet

## Just Finished

Step 2 - Build The Freeze Closure Ledger completed for the current targets in `plan.md`.

Freeze closure ledger:

| Target | Generated metadata-rich path | Structured authority | Retained string boundary | Closure classification |
| --- | --- | --- | --- | --- |
| Direct-call signature metadata | `LirCallOp.direct_callee_link_name_id` plus `LirCallOp.callee_signature`; `LirFunction.signature_return_type_ref`, `signature_params`, `signature_param_type_refs`, `signature_is_variadic`, and `signature_has_void_param_list`; BIR call ABI lowering via structured signature params/return refs | `LinkNameId` for direct callee identity; `LirCallSignature`, `LirSignatureParam`, `LirTypeRef`, `StructNameId`, target ABI facts, and `BackendStructuredLayoutTable` for return, param, byval, sret, variadic, and aggregate layout facts | `signature_text`, rendered call argument fragments, and parsed signature helpers are final LLVM/output spelling or explicit raw/no-metadata compatibility; BIR `return_type_name`, `structured_return_type_name`, and printer byval/sret suffixes are display/output | Closed for generated paths: structured metadata is validated and stale/missing aggregate metadata fails closed where the selected target requires it. No generated metadata-rich direct-call path is classified as using rendered signature text as semantic authority. |
| Generated direct-call symbol path | LIR direct call target lowering through `direct_callee_link_name_id`; BIR `CallInst.callee_link_name_id`; validator `find_function_by_link_name_id` and mismatch checks | `LinkNameId` and the module link-name table own user/extern function identity | `CallInst.callee` remains printed callee spelling, diagnostics context, raw/no-id compatibility, and runtime/intrinsic placeholder token when `callee_link_name_id` is intentionally invalid | Closed for generated user/extern calls: a stale visible callee name cannot override `LinkNameId`. Runtime/intrinsic placeholders are classified compatibility/display, not ordinary symbol identity. |
| Global and type declaration tables | Generated aggregate-global `llvm_type_ref` and initializer lowering through `LirTypeRef`/`StructNameId`; `build_backend_structured_layout_table`; `lookup_backend_aggregate_type_ref_layout_result`; `lower_aggregate_initializer_for_type_ref` | `StructNameId`, `LirTypeRef`, structured LIR declarations, and `BackendStructuredLayoutTable` own aggregate layout identity; `LinkNameId` owns global symbol identity where available | `TypeDeclMap`, `GlobalTypes`, final type spelling, and textual initializers remain raw/no-id compatibility or final/display spelling; structured declaration spellings bridge ids to legacy layouts but are not the semantic owner | Closed for the selected generated aggregate-global/type path: stale text-vs-id or missing `StructNameId` rejects instead of silently choosing final spelling. Legacy tables remain classified compatibility. |
| Direct symbol identity validation surfaces | BIR `Global.link_name_id`, `Function.link_name_id`, `LoadGlobalInst.global_name_id`, `StoreGlobalInst.global_name_id`, `Global.initializer_symbol_name_id`, `Value.pointer_symbol_link_name_id`, and pointer initializer structured symbol references | `LinkNameId` plus module validation (`validate_link_name_id`, `find_global`, `find_function`, initializer symbol validation, named pointer validation) own direct global/function identity | Visible `global_name`, function/global `name`, initializer symbol spelling, and value `name` remain display/diagnostic/raw compatibility carriers; string-pool constants intentionally have no `LinkNameId` and are explicit no-metadata compatibility | Closed for generated direct symbols: mismatched ids/names and missing structured ids fail closed for covered generated user/extern symbol paths. Explicit invalid-id compatibility remains classified. |
| Memory provenance global handles | Addressed-global pointer provenance through `AddressedGlobalPointerSlots`, `AddressedGlobalPointerValueSlots`, `Value.pointer_symbol_link_name_id`, lowering `lir_to_bir_detail::GlobalAddress::link_name_id`, BIR `Address::base_link_name_id`, and BIR memory provenance resolution | `LinkNameId` keys the selected addressed-global provenance path; structured global refs carry `pointer_symbol_link_name_id`, lowering `GlobalAddress::link_name_id`, or BIR address `base_link_name_id` and validate against declared global/function ids | Local slot names, SSA names, temporary names, pointer alias labels, dynamic-array locals, and raw-import global spellings are route-local handles or no-metadata compatibility, not semantic global authority | Closed for the selected global-provenance path: addressed-global provenance no longer treats final global spelling as ordinary semantic identity when id metadata exists. Route-local strings remain explicitly retained. |
| Backend prealloc route-local naming | Prealloc prepared forms (`PreparedNameTables`, `PreparedValueId`, `PreparedStackObject`, `PreparedAddress`, call plans, liveness/regalloc/storage/frame plans); direct symbol-backed addresses use BIR `LinkNameId` when present | Prepared ids and structured prepared records own route-local analysis identity; global-symbol addresses resolve through link-name ids and reject stale raw/id pairings | Prepared printer strings, value names, block labels, stack object source names, register names, `direct_callee_name`, and same-module render-contract names are display/output or route-local handles. Fallback raw names exist only when no structured id is available | Classified as retained route-local/output boundary. No evidence that prealloc route-local names are acting as cross-module semantic symbol authority on generated metadata-rich paths. |

Retained string boundary summary:

- Compatibility: raw/no-id LIR/BIR imports, no-metadata direct-call signature parsing, legacy `TypeDeclMap`/`GlobalTypes` lookup, string-pool names, runtime/intrinsic placeholder calls, and unresolved initializer/string constants with invalid ids.
- Diagnostics: validator contexts and mismatch messages use visible names to explain failed id/name pairings.
- Display/output: LIR `signature_text`, BIR printers, prepared printers, final type spelling, function/global visible names, byval/sret suffix text, and emitted symbol spelling.
- ABI/final spelling: LLVM function headers, call argument fragments, aggregate byval/sret textual fragments, and object/assembler-facing names remain final spelling carriers, not structured identity owners.
- Route-local handles: local slots, SSA values, temporaries, block labels, stack objects, pointer aliases, prepared value/block/register names, sret storage names, and prealloc render-contract carrier names.
- Explicit no-metadata boundaries: invalid `LinkNameId`/`StructNameId`/slot-id paths are allowed only for legacy/raw imports, placeholders, or compatibility inputs that lack the structured producer carrier.

High-risk generated-path assessment:

- No current in-scope generated metadata-rich path still appears to use rendered text as semantic authority.
- Remaining rendered text is classified as output/display, diagnostics, route-local naming, ABI/final spelling, or explicit no-metadata compatibility.
- The earlier 185 pointer-initializer follow-up territory is covered for this gate by 186/187 surfaces: `initializer_function_link_name_ids`, `initializer_symbol_name_id`, and `Value.pointer_symbol_link_name_id` now provide structured symbol validation, while unresolved initializer spelling remains explicit compatibility.

## Suggested Next

Delegate Step 3: run milestone validation with the supervisor-selected broad command, normally a full suite or regression-guard equivalent. Record the exact command, canonical log handling, green result or accepted baseline difference, and confirm that no expectation downgrade or narrow-only proof is being used to clear the closure gate.

## Watchouts

- This is a closure gate, not backend restart implementation.
- Do not downgrade testcase expectations or weaken contracts as freeze progress.
- 187 rejected an unrelated full-suite baseline candidate; Step 3 still needs supervisor-selected milestone validation rather than relying on that rejected candidate.
- The Step 2 ledger is evidence/classification only. Step 3 still owns milestone proof; Step 4 still owns the backend-restart clear-or-blocked decision.
- Keep intermediate findings in `todo.md`; source idea edits are only for durable intent changes or a new blocker initiative.

## Proof

Ledger-only packet. No build/test proof required by delegation, and no proof logs were created or modified.
