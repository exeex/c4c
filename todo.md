Status: Active
Source Idea Path: ideas/open/183_lir_bir_backend_freeze_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory LIR/BIR identity surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for LIR/BIR/backend-prealloc identity surfaces.

Freeze-ledger inventory:

| Domain | Surface | Stored spelling/id fields | Source references | Current role / notes |
| --- | --- | --- | --- | --- |
| LIR symbol tables | `LirModule::link_name_texts`, `link_names`, `struct_names` | `LinkNameId`, `StructNameId`, shared text storage | `src/codegen/lir/ir.hpp:623`, `src/codegen/lir/ir.hpp:627`, `src/codegen/lir/ir.hpp:628` | Semantic source tables for link-visible names and structured type names on generated paths. |
| LIR direct calls | `LirCallOp` plus `LirCallSignature` | `callee.str()`, `direct_callee_link_name_id`, `callee_type_suffix`, `args_str`, `arg_type_refs`, `callee_signature`, signature type refs | `src/codegen/lir/ir.hpp:277`, `src/codegen/lir/ir.hpp:288`, `src/codegen/lir/ir.hpp:292`, `src/codegen/lir/ir.hpp:295`, `src/codegen/lir/ir.hpp:296` | Mixed boundary: direct callee id is semantic when present; signature/arg strings remain compatibility and parsing input, with structured mirrors available for generated aggregate args/results. |
| LIR functions/externs | `LirFunction`, `LirExternDecl`, extern dedup maps | final `name`, `link_name_id`, `signature_text`, `signature_*_type_ref`, `extern_decl_link_name_map`, `extern_decl_name_map` | `src/codegen/lir/ir.hpp:500`, `src/codegen/lir/ir.hpp:514`, `src/codegen/lir/ir.hpp:525`, `src/codegen/lir/ir.hpp:526`, `src/codegen/lir/ir.hpp:539`, `src/codegen/lir/ir.hpp:644` | `LinkNameId` is the semantic identity. `signature_text` and name-map dedup are retained output/no-metadata compatibility surfaces. Comment notes function refs inside `signature_text` have no structured producer carrier yet. |
| LIR globals/initializers | `LirGlobal` and global initializer metadata | final `name`, `link_name_id`, `llvm_type`, `llvm_type_ref`, `init_text`, `initializer_function_link_name_ids` | `src/codegen/lir/ir.hpp:554`, `src/codegen/lir/ir.hpp:557`, `src/codegen/lir/ir.hpp:566`, `src/codegen/lir/ir.hpp:571`, `src/codegen/lir/ir.hpp:572` | Global identity uses `LinkNameId`; `init_text` scanning is a compatibility fallback for legacy initializer producers, while function refs in initializers can carry semantic ids. |
| LIR type/layout facts | `LirTypeRef`, `LirStructDecl`, structured layout observations | rendered text, `LirTypeKind`, `StructNameId`, `struct_decls`, `struct_decl_index`, parity observation fields | `src/codegen/lir/types.hpp:33`, `src/codegen/lir/types.hpp:54`, `src/codegen/lir/types.hpp:72`, `src/codegen/lir/types.hpp:90`, `src/codegen/lir/ir.hpp:583`, `src/codegen/lir/ir.hpp:590`, `src/codegen/lir/ir.hpp:721`, `src/codegen/lir/ir.hpp:728` | `StructNameId` wins equality when both sides carry it; rendered type text is still final spelling and legacy/layout compatibility input. |
| LIR verification | type/id mirror validation helpers | `require_type_ref`, `StructNameId` mirror checks, direct aggregate signature checks | `src/codegen/lir/verify.cpp:53`, `src/codegen/lir/verify.cpp:118`, `src/codegen/lir/verify.cpp:736`, `src/codegen/lir/verify.cpp:787`, `src/codegen/lir/verify.cpp:986` | Source-level guard surface ensuring generated type mirrors remain consistent with rendered text and declared struct ids. |
| BIR name tables | `bir::NameTables` | `LinkNameTable`, `BlockLabelTable`, `SlotNameTable` | `src/backend/bir/bir.hpp:17`, `src/backend/bir/bir.hpp:60`, `src/backend/bir/bir.hpp:67` | BIR imports LIR link-name tables and adds route-local block/slot ids. |
| BIR values/symbol pointers | `bir::Value` | display `name`, `pointer_symbol_link_name_id` | `src/backend/bir/bir.hpp:127`, `src/backend/bir/bir.hpp:133`, `src/backend/bir/bir.cpp:74` | Named value text is display/SSA spelling except pointer values denoting globals/functions, where `pointer_symbol_link_name_id` is semantic when valid. |
| BIR globals/functions | `bir::Global`, `bir::Function` | final `name`, `link_name_id`, initializer symbol text/id | `src/backend/bir/bir.hpp:219`, `src/backend/bir/bir.hpp:221`, `src/backend/bir/bir.hpp:229`, `src/backend/bir/bir.hpp:235`, `src/backend/bir/bir.hpp:521`, `src/backend/bir/bir.hpp:525` | Link id is semantic identity; initializer symbol text is compatibility/display unless resolved to `initializer_symbol_name_id`. |
| BIR calls and memory ops | `CallInst`, `MemoryAddress`, `LoadGlobalInst`, `StoreGlobalInst`, local slot accesses | `callee`, `callee_link_name_id`, `callee_value`, `sret_storage_name_id`, `base_link_name_id`, `base_label_id`, `base_slot_id`, `global_name_id`, `slot_id` | `src/backend/bir/bir.hpp:280`, `src/backend/bir/bir.hpp:288`, `src/backend/bir/bir.hpp:372`, `src/backend/bir/bir.hpp:378`, `src/backend/bir/bir.hpp:396`, `src/backend/bir/bir.hpp:399`, `src/backend/bir/bir.hpp:417`, `src/backend/bir/bir.hpp:429` | Calls/globals/memory carry ids when known; raw names remain accepted for unresolved compatibility and dump/display routes. |
| BIR validation | validator lookup and id/name consistency checks | `validate_link_name_id`, `find_global`, `find_function`, duplicate id checks | `src/backend/bir/bir_validate.cpp:20`, `src/backend/bir/bir_validate.cpp:42`, `src/backend/bir/bir_validate.cpp:347`, `src/backend/bir/bir_validate.cpp:360`, `src/backend/bir/bir_validate.cpp:409`, `src/backend/bir/bir_validate.cpp:723` | Validation already fails mismatched present ids against visible names and declared globals/functions. Raw-name lookup still exists for invalid-id compatibility. |
| LIR-to-BIR function identity | `LirFunctionIdentityLookup`, direct-call collection/repair | `by_link_name_id`, `fallback_by_name`, BIR/LIR direct call lists | `src/backend/bir/lir_to_bir.cpp:202`, `src/backend/bir/lir_to_bir.cpp:216`, `src/backend/bir/lir_to_bir.cpp:223`, `src/backend/bir/lir_to_bir.cpp:238` | Semantic function match by `LinkNameId`; fallback by final name only for raw-only LIR functions. |
| LIR-to-BIR globals/types | `GlobalTypes`, `TypeDeclMap`, `FunctionSymbolSet`, structured layout table | raw string keys, `GlobalInfo::link_name_id`, structured layout `name_id`, legacy/structured parity flags | `src/backend/bir/lir_to_bir/lowering.hpp:52`, `src/backend/bir/lir_to_bir/lowering.hpp:78`, `src/backend/bir/lir_to_bir/lowering.hpp:84`, `src/backend/bir/lir_to_bir/lowering.hpp:138`, `src/backend/bir/lir_to_bir/lowering.hpp:157`, `src/backend/bir/lir_to_bir/module.cpp:904`, `src/backend/bir/lir_to_bir/module.cpp:920`, `src/backend/bir/lir_to_bir/module.cpp:924` | Explicit compatibility tables keyed by producer final spellings. Generated BIR output should carry ids where metadata exists; type-decl text remains a compatibility/layout bridge. |
| LIR-to-BIR direct-call signatures | `parse_direct_global_typed_call`, byval layout lowering | parsed callee symbol string, param type strings, `arg_type_refs`, `StructNameId` checks | `src/backend/bir/lir_to_bir/calling.cpp:207`, `src/backend/bir/lir_to_bir/calling.cpp:247`, `src/backend/bir/lir_to_bir/calling.cpp:582`, `src/backend/bir/lir_to_bir/calling.cpp:590`, `src/backend/bir/lir_to_bir/calling.cpp:599` | Still parses rendered signature/arg text. Structured arg mirrors fence generated aggregate byval paths; absence of mirrors intentionally falls back to legacy text. |
| LIR-to-BIR pointer initializers | global and aggregate initializer lowering | `initializer_symbol_name`, `initializer_function_link_name_id`, pointer offset maps keyed by byte offset | `src/backend/bir/lir_to_bir/globals.cpp:55`, `src/backend/bir/lir_to_bir/globals.cpp:85`, `src/backend/bir/lir_to_bir/globals.cpp:132`, `src/backend/bir/lir_to_bir/globals.cpp:142`, `src/backend/bir/lir_to_bir/globals.cpp:443`, `src/backend/bir/lir_to_bir/module.cpp:119`, `src/backend/bir/lir_to_bir/module.cpp:958` | Present initializer ids fail closed through `resolve_initializer_symbol_link_name_id`; raw symbol fallback is retained only when no id exists. |
| LIR-to-BIR memory provenance | global/pointer/address maps | `GlobalAddress::global_name/link_name_id`, `GlobalPointerSlotKey`, local slot maps keyed by SSA/slot strings | `src/backend/bir/lir_to_bir/lowering.hpp:43`, `src/backend/bir/lir_to_bir/lowering.hpp:118`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:20`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:85`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:116`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:154` | Global provenance carries `LinkNameId` after resolution. Function-local SSA/slot maps are route-local handles, not module semantic identity. |
| Prealloc name tables | `PreparedNameTables` and lookup helpers | function/block/value/slot/link tables, prepared ids | `src/backend/prealloc/prealloc.hpp:40`, `src/backend/prealloc/prealloc.hpp:91`, `src/backend/prealloc/prealloc.hpp:114`, `src/backend/prealloc/prealloc.hpp:120`, `src/backend/prealloc/prealloc.hpp:126`, `src/backend/prealloc/prealloc.hpp:141`, `src/backend/prealloc/prealloc.hpp:161` | Prepared ids are route-local publication handles for backend preparation. `link_names` is retained for same-module/global-symbol references. |
| Prealloc addressing/calls/storage | prepared address, memory access, call/storage plans | `symbol_name`, `source_symbol_name/source_symbol_name_id`, `PreparedComputedBase` global ids, storage `symbol_name` | `src/backend/prealloc/prealloc.hpp:430`, `src/backend/prealloc/prealloc.hpp:441`, `src/backend/prealloc/prealloc.hpp:944`, `src/backend/prealloc/prealloc.hpp:952`, `src/backend/prealloc/prealloc.hpp:1105`, `src/backend/prealloc/prealloc.hpp:1390` | Route-local plans carry prepared ids plus display strings. Symbol ids should be preferred where present; register names are target/final spelling. |
| Prealloc same-module globals | materialized compare/join render contracts | `PreparedSameModuleGlobalRef::name/name_id`, `same_module_global_refs`, `bir_link_name_or_raw` | `src/backend/prealloc/prealloc.hpp:1801`, `src/backend/prealloc/prealloc.hpp:1827`, `src/backend/prealloc/prealloc.hpp:3778`, `src/backend/prealloc/prealloc.hpp:3791` | Same-module globals can be resolved by prepared link id; raw-name fallback remains for invalid-id compatibility. |
| Prealloc CFG/SSA names | out-of-SSA and control-flow helpers | preferred block labels, value names, slot names, join-transfer ids | `src/backend/prealloc/out_of_ssa.cpp:36`, `src/backend/prealloc/out_of_ssa.cpp:69`, `src/backend/prealloc/out_of_ssa.cpp:117`, `src/backend/prealloc/out_of_ssa.cpp:550`, `src/backend/prealloc/out_of_ssa.cpp:973`, `src/backend/prealloc/out_of_ssa.cpp:1065` | Block/value/slot names are route-local identity for prepared control flow and phi materialization; `preferred_bir_block_label` uses BIR ids when available, otherwise raw labels. |
| Prealloc stack layout | symbol-backed prepared addresses | fallback symbol text/id, prepared `LinkNameId` intern | `src/backend/prealloc/stack_layout/coordinator.cpp:247`, `src/backend/prealloc/stack_layout/coordinator.cpp:256`, `src/backend/prealloc/stack_layout/coordinator.cpp:304` | Present BIR link ids are checked against display text before interning; raw fallback accepted only when no id is available. |

Uncertain / follow-up classification targets for Step 2:

- Direct-call signature parsing still has multiple rendered-text entry points
  (`callee_type_suffix`, `args_str`, parsed byval text). Generated aggregate
  paths appear fenced by `arg_type_refs`/`StructNameId`, but direct-call
  signature authority needs explicit classification against idea 184.
- `LirFunction::signature_text` notes unresolved producer-boundary function
  references. This is an inventory item for idea 184/188 classification, not a
  fix in this audit.
- `GlobalTypes` and `TypeDeclMap` are intentionally string-keyed compatibility
  tables. Step 2 should verify every generated metadata-rich path entering
  them has a structured/id carrier or is owned by idea 185.
- Memory provenance keeps route-local SSA/slot string maps while also carrying
  `LinkNameId` for global handles. Step 2 should separate local storage handles
  from global semantic authority and map remaining cleanup to idea 187.
- Prealloc uses prepared function/block/value/slot ids as route-local backend
  handles. Same-module global and symbol-backed address paths retain raw
  fallback behavior; Step 2 should classify those fallbacks as compatibility
  only unless a present id is missing or ignored.

## Suggested Next

Proceed to Step 2 classification: turn this inventory into authority categories
and map any generated metadata-rich text fallback to existing ideas 184-188 or
a blocker note.

## Watchouts

- This active plan is an audit, not a backend implementation slice.
- Do not treat every retained string as a defect; classify display, diagnostic,
  output, compatibility, and route-local uses separately from semantic
  authority.
- Ideas 184-188 already exist as likely follow-up owners; only propose a new
  source idea if a blocker is genuinely uncovered.

## Proof

Source-level audit only. Ran `git diff --check -- todo.md`; no build/tests were
run because only `todo.md` changed.
