Status: Active
Source Idea Path: ideas/open/183_lir_bir_backend_freeze_authority_audit.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify authority and compatibility boundaries

# Current Packet

## Just Finished

Step 2 classification completed for the Step 1 LIR/BIR/backend-prealloc
identity inventory.

Freeze-ledger classification:

| Domain | Authority / compatibility classification | Evidence | Follow-up owner |
| --- | --- | --- | --- |
| Direct-call signatures | High-risk generated-path fallback. `LirCallOp` has structured mirrors (`direct_callee_link_name_id`, `arg_type_refs`, `callee_signature`), but direct lowering still accepts rendered signature pieces through `parse_direct_global_typed_call`, byval type parsing, and `args_str`/`callee_type_suffix`. Retained text is valid only for display/final spelling or explicit no-metadata compatibility. | `src/codegen/lir/ir.hpp:290`, `src/codegen/lir/ir.hpp:292`, `src/codegen/lir/ir.hpp:295`, `src/codegen/lir/ir.hpp:296`, `src/backend/bir/lir_to_bir/calling.cpp:248`, `src/backend/bir/lir_to_bir/calling.cpp:582`, `src/backend/bir/lir_to_bir/calling.cpp:590` | Covered by `ideas/open/184_direct_call_signature_metadata_structured_boundary.md`. |
| Direct callee identity | Semantic authority is `LinkNameId` when present. Final callee strings are ABI/final spelling, diagnostics, runtime/intrinsic placeholder spelling, or raw/no-id compatibility only. BIR validation already checks many present-id/name pairings, but generated direct-symbol fail-closed closure remains high risk. | `src/codegen/lir/ir.hpp:292`, `src/backend/bir/bir.hpp:376`, `src/backend/bir/bir.hpp:378`, `src/backend/bir/bir_validate.cpp:213`, `src/backend/bir/bir_validate.cpp:216`, `src/backend/bir/bir_validate.cpp:220`, `src/backend/bir/bir_validate.cpp:228` | Covered by `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`; final gate covered by 188. |
| Globals and pointer-initializer symbols | Semantic authority is `LinkNameId` for generated globals/functions and pointer initializer references when present. `initializer_symbol_name`, `init_text`, and raw symbol lookup are compatibility/display bridges and must not become string-first authority on metadata-rich paths. | `src/codegen/lir/ir.hpp:557`, `src/codegen/lir/ir.hpp:568`, `src/codegen/lir/ir.hpp:572`, `src/backend/bir/bir.hpp:232`, `src/backend/bir/bir.hpp:235`, `src/backend/bir/bir_validate.cpp:412`, `src/backend/bir/bir_validate.cpp:421`, `src/backend/bir/bir_validate.cpp:433`, `src/backend/bir/lir_to_bir/globals.cpp:132`, `src/backend/bir/lir_to_bir/globals.cpp:137`, `src/backend/bir/lir_to_bir/globals.cpp:150` | Covered by `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`; GlobalTypes interactions also covered by 185. |
| `GlobalTypes` compatibility table | Explicit raw/final-spelling compatibility table. `GlobalInfo::link_name_id` carries semantic identity, while the map key remains producer spelling for raw import and legacy lookup. Generated metadata-rich use of the string key is a freeze blocker unless fenced to `LinkNameId`. | `src/backend/bir/lir_to_bir/lowering.hpp:43`, `src/backend/bir/lir_to_bir/lowering.hpp:46`, `src/backend/bir/lir_to_bir/lowering.hpp:47`, `src/backend/bir/lir_to_bir/lowering.hpp:78`, `src/backend/bir/lir_to_bir/lowering.hpp:82`, `src/backend/bir/lir_to_bir/globals.cpp:150` | Covered by `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`. |
| `TypeDeclMap` and structured layout tables | `TypeDeclMap` is a legacy rendered-type compatibility bridge. `LirTypeRef`, `StructNameId`, and `BackendStructuredLayoutEntry::name_id` are the structured authorities for generated aggregate layout facts. Layout lookups that accept text are acceptable for raw/no-id compatibility; generated metadata-rich paths must prefer structured facts or fail closed. | `src/codegen/lir/types.hpp:54`, `src/codegen/lir/types.hpp:72`, `src/codegen/lir/types.hpp:90`, `src/backend/bir/lir_to_bir/lowering.hpp:83`, `src/backend/bir/lir_to_bir/lowering.hpp:161`, `src/backend/bir/lir_to_bir/lowering.hpp:170`, `src/backend/bir/lir_to_bir/globals.cpp:103`, `src/backend/bir/lir_to_bir/globals.cpp:123` | Covered by `ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md`. |
| Aggregate layout facts in call/global/memory lowering | Semantic authority is structured layout metadata where available (`StructNameId`, `LirTypeRef`, `BackendStructuredLayoutTable`). Rendered aggregate text can remain output spelling and compatibility input. Direct call byval/sret and global aggregate initializer consumers are high-risk where text lookup remains in the normal path. | `src/codegen/lir/verify.cpp:736`, `src/codegen/lir/verify.cpp:787`, `src/codegen/lir/verify.cpp:894`, `src/backend/bir/lir_to_bir/calling.cpp:371`, `src/backend/bir/lir_to_bir/calling.cpp:599`, `src/backend/bir/lir_to_bir/global_initializers.cpp:206`, `src/backend/bir/lir_to_bir/memory/addressing.cpp:883` | Covered by 184 for direct calls, 185 for global/type/layout tables, and 188 for final freeze closure. |
| BIR direct global load/store identity | `LoadGlobalInst::global_name_id` and `StoreGlobalInst::global_name_id` are semantic when present. `global_name` is final spelling/display and raw compatibility. Validator lookup accepts raw names only when ids are invalid; metadata-rich generated paths should not rely on that fallback. | `src/backend/bir/bir.hpp:417`, `src/backend/bir/bir.hpp:429`, `src/backend/bir/bir_validate.cpp:526`, `src/backend/bir/bir_validate.cpp:530`, `src/backend/bir/bir_validate.cpp:584`, `src/backend/bir/bir_validate.cpp:588` | Covered by `ideas/open/186_bir_direct_symbol_identity_validation_closure.md`. |
| BIR pointer values and symbol pointers | `Value::pointer_symbol_link_name_id` is semantic for symbol pointer values. `Value::name` remains SSA/display spelling except for named symbol pointer compatibility; mismatched present ids are validation failures. | `src/backend/bir/bir.hpp:127`, `src/backend/bir/bir.hpp:133`, `src/backend/bir/bir.cpp:76`, `src/backend/bir/bir_validate.cpp:49`, `src/backend/bir/bir_validate.cpp:64`, `src/backend/bir/bir_validate.cpp:76` | Covered by `ideas/open/186_bir_direct_symbol_identity_validation_closure.md` and memory cleanup in 187. |
| Memory provenance global handles | Global provenance should be `LinkNameId`-backed (`GlobalAddress::link_name_id`, `link_name_id_for_global`, `known_global_address`) where possible. `global_name` and string-keyed provenance maps are compatibility bridges until resolved; local SSA/slot/pointer maps are route-local handles, not module symbol authority. | `src/backend/bir/lir_to_bir/lowering.hpp:43`, `src/backend/bir/lir_to_bir/lowering.hpp:47`, `src/backend/bir/lir_to_bir/lowering.hpp:118`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:20`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:141`, `src/backend/bir/lir_to_bir/memory/provenance.cpp:303`, `src/backend/bir/lir_to_bir/memory/local_slots.cpp:1256` | Covered by `ideas/open/187_bir_memory_provenance_global_handle_cleanup.md`; depends on 185/186. |
| Prealloc route-local function/block/value/slot names | Prepared function, block, value, slot, frame-slot, and join-transfer ids are route-local backend-preparation handles. Their strings can remain route-local labels or diagnostics. They are not LIR/BIR semantic symbol authority unless they reference `LinkNameId` tables. | `src/backend/prealloc/prealloc.hpp:40`, `src/backend/prealloc/prealloc.hpp:91`, `src/backend/prealloc/out_of_ssa.cpp:36`, `src/backend/prealloc/out_of_ssa.cpp:69`, `src/backend/prealloc/regalloc.cpp:1589`, `src/backend/prealloc/stack_layout/coordinator.cpp:77` | No separate blocker found; closure confirmation belongs to `ideas/open/188_lir_bir_freeze_closure_gate.md`. |
| Prealloc symbol-backed addresses and same-module globals | `LinkNameId`/prepared link-name ids are semantic when present. Raw `symbol_name`, `source_symbol_name`, and `bir_link_name_or_raw` fallbacks are raw/no-id compatibility or prepared-printer output. The stack-layout path already rejects mismatched present ids against spelling before interning. | `src/backend/prealloc/prealloc.hpp:430`, `src/backend/prealloc/prealloc.hpp:944`, `src/backend/prealloc/prealloc.hpp:1801`, `src/backend/prealloc/prealloc.hpp:3778`, `src/backend/prealloc/stack_layout/coordinator.cpp:247`, `src/backend/prealloc/stack_layout/coordinator.cpp:256`, `src/backend/prealloc/stack_layout/coordinator.cpp:304`, `src/backend/prealloc/prepared_printer.cpp:65` | No uncovered blocker found; direct symbol/global closure belongs to 186 and final confirmation to 188. |

High-risk generated metadata-rich text fallbacks and owners:

| Fallback | Classification | Owner / blocker note |
| --- | --- | --- |
| Direct-call rendered signature parsing for generated calls | Freeze blocker unless the path requires structured call signature facts and reserves parsing for explicit no-metadata inputs. | Owned by 184. |
| Direct-call aggregate byval/sret/layout text lookup | Freeze blocker when generated aggregate metadata exists but lowering trusts rendered fragments. | Owned by 184; shared layout table boundary owned by 185. |
| `GlobalTypes` string-keyed global lookup on generated globals | Freeze blocker unless treated as raw/no-id compatibility and cross-checked with `LinkNameId`. | Owned by 185. |
| `TypeDeclMap` and rendered aggregate type lookup on generated metadata-rich layout paths | Freeze blocker unless structured `LirTypeRef`/`StructNameId`/layout facts are authoritative. | Owned by 185. |
| BIR direct call/global/pointer-initializer raw symbol fallback with available ids | Freeze blocker if generated paths accept the fallback as semantic identity. | Owned by 186. |
| Memory/provenance global maps keyed by final spelling after `LinkNameId` is available | Freeze blocker if used as the global authority instead of a bridge to `GlobalAddress::link_name_id`. | Owned by 187. |
| Prealloc raw symbol fallback for same-module or symbol-backed prepared addresses | Compatibility/display only when no valid id exists; closure gate should verify this remains fenced. | Covered by 186/188; no new blocker idea found. |

No uncovered high-risk generated metadata-rich text fallback was found in this
Step 2 classification. The known blocker families are covered by existing open
ideas 184-188.

## Suggested Next

Proceed to Step 3: map the classified blocker families to ideas 184-188 in a
short closeout matrix and confirm whether any additional open idea is needed
before backend restart.

## Watchouts

- This audit classifies authority boundaries only; do not implement backend,
  lowering, validator, prealloc, or test changes inside idea 183.
- Treat local SSA, block, slot, prepared-frame, and register strings as
  route-local or final target spelling unless they cross into global symbol
  identity.
- Ideas 184-187 own the specific repair families; 188 owns the milestone gate
  after those repairs complete.

## Proof

Source-level audit only. Used `rg` plus targeted `c4c-clang-tool-ccdb
function-signatures` queries for direct-call lowering, global lowering,
memory/provenance, and prealloc stack-layout symbol handling. Ran
`git diff --check -- todo.md`; no build/tests were run because only `todo.md`
changed.
