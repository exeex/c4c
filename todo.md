Status: Active
Source Idea Path: ideas/open/188_lir_bir_freeze_closure_gate.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Audit For Remaining High-Risk Generated-Path Text Authority

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by auditing the freeze ledger's uncertain generated-path string-authority surfaces against current code. No high-risk generated-path string authority remains unclassified, and no narrow blocker idea is needed before milestone validation.

Audit classifications:

- Pointer initializer address/global resolution is classified as structured authority for metadata-rich generated paths plus raw/no-id compatibility for imported fixtures. `resolve_initializer_symbol_link_name_id(...)`, `resolve_known_global_address(...)`, and `resolve_pointer_initializer_offsets(...)` keep present `initializer_function_link_name_ids` authoritative and fail closed when the id is unknown instead of falling back to raw spelling. Global pointer initializer fields resolve through canonical `global_types` entries that were populated from `global_name_for_identity(...)`; aggregate pointer initializer elements are rewritten to `bir::Value::named_symbol_pointer(..., LinkNameId)` when a resolved target id exists. Existing backend coverage exercises drift success, missing-id fail-closed behavior, aggregate function/global pointer initializers, and raw/no-id compatibility.
- Remaining memory/provenance `global_types.find(global_name)` and `GlobalAddress` flows are classified as route-local/no-id compatibility or structured handoff surfaces, not unclassified semantic text authority. Direct LIR global operands still use raw spelling to enter the route, but `GlobalAddress` carries `link_name_id`, `make_global_pointer_slot_key(...)` includes `LinkNameId`, `LoadGlobalInst`/`StoreGlobalInst` outputs carry `global_name_id`, addressed-global pointer slots are keyed by structured identity from idea 187, and dynamic global scalar arrays are LinkNameId-checked from idea 194. Remaining dynamic pointer/aggregate array and ptrtoint/inttoptr maps use SSA result strings or global spelling as local map handles while materialized symbol values and BIR global instructions carry the available structured id; absent ids remain explicit raw/no-id compatibility.
- Prealloc route-local names are classified as route-local handles, structured-id lookups, or presentation/final spelling. `PreparedNameTables` interns function, value, slot, block, and link names into prepared ids; prealloc call, frame, liveness, regalloc, addressing, and prepared-printer surfaces carry those prepared ids internally. Raw function/value/slot names are used to locate BIR-local values, match BIR blocks, or render prepared diagnostics/output. Symbol pointers and global loads are guarded by `pointer_symbol_link_name_id`, `callee_link_name_id`, `global_name_id`, and prepared link-name refs where semantic cross-function/global identity matters.

Conclusion: the three Step 2 uncertain surfaces are classified. No remaining high-risk metadata-rich generated path was found that still treats rendered text as semantic function/global/type authority.

## Suggested Next

Execute Step 3 by running the supervisor-selected milestone validation command, normally a full build plus full CTest suite with canonical regression logs if requested.

## Watchouts

- Do not start backend restart work inside this gate.
- This audit did not patch implementation or tests.
- Milestone validation should be broad; the audit-only result depends on the supervisor selecting the proof scope.
- Retained raw strings remain acceptable only as presentation/output, diagnostics, route-local handles, ABI/final spelling, or explicit no-id compatibility.

## Proof

Audit-only packet; no build or test command was required or run. Read-only inspection used `rg`, targeted file reads, and `c4c-clang-tool-ccdb function-signatures` for `globals.cpp`, `memory/provenance.cpp`, and `prealloc/regalloc.cpp`. Existing `test_after.log` was left untouched.
