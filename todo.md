Status: Active
Source Idea Path: ideas/open/167_whole_codebase_string_authority_final_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Route-Local and Generated Names

# Current Packet

## Just Finished

Step 3 - Classify Route-Local and Generated Names first packet is complete.
No implementation or test files were edited.

Classification table:

| Owner/domain | File/local symbol or path | Class | Evidence | Local collision domain / stage boundary | Generator for generated names | Feed idea 169? |
| --- | --- | --- | --- | --- | --- | --- |
| LIR SSA values | `src/codegen/lir/ir.hpp` `Lir*Op.result`, `LirOperand`, `LirFunction::alloc_value`; `src/codegen/lir/hir_to_lir/core.cpp` `fresh_tmp` | `RL` / `GT` | LIR result comments call these SSA names; `fresh_tmp` emits `%t<N>` from per-function `FnCtx::tmp_idx`; values render in `lir_printer.cpp` as final LLVM text. | One LIR function; crosses HIR-to-LIR into final LLVM rendering, not source/link identity. | `StmtEmitter::fresh_tmp`: `%t` plus monotonically increasing `ctx.tmp_idx`; a few derived suffixes such as `.agg`, `.addrtmp`, `.bf.shl` stay in the same function. | Yes: local value identity should become typed/local ids where later backend stages synchronize multiple maps by spelling. |
| LIR block labels | `src/codegen/lir/ir.hpp` `LirBlock::label`, terminator labels, phi incoming labels; `src/codegen/lir/hir_to_lir/core.cpp` `fresh_lbl` | `RL` / `GT` | Terminators and phi pairs store raw labels; `fresh_lbl(ctx, pfx)` emits prefix plus `ctx.tmp_idx`; `lir_printer.cpp` renders `%label` targets. | One LIR function; crosses HIR-to-LIR into LIR-to-BIR CFG lowering. | `StmtEmitter::fresh_lbl` with route prefixes like `while.cond.`, `logic.end.`, `vaarg.*`, plus stable source block labels from `block_lbl`. | Yes: LIR-to-BIR currently consumes spellings before `BlockLabelId` assignment. |
| LIR string pool/private globals | `src/codegen/lir/ir.hpp` `LirStringConst`, `LirModule::intern_str`, `str_pool_map`, `string_pool`; `src/codegen/lir/hir_to_lir/expr/coordinator.cpp` direct wide-string path | `GT` / `CB` | `intern_str` dedups by raw bytes and emits `@.str<N>`; `expr/coordinator.cpp` has a direct `@.str<N>` wide-literal path; `lir_printer.cpp` renders these as `private unnamed_addr constant`; BIR lowering strips leading `@` and keeps name-only identity. | One LIR module string-pool namespace; crosses LIR-to-BIR as addressable private data but not source `LinkNameId`. | `LirModule::intern_str`: `@.str` plus `str_pool_idx++`; wide string path also increments `module_->str_pool_idx`. | Yes: generated addressable data names are route-local/generated and need separation from source/link-visible globals. |
| LIR final LLVM payloads | `LirFunction::signature_text`, `LirGlobal::init_text`, `lir_printer.cpp` `render_fn`/`print_llvm`, `verify.cpp` signature compatibility parser | `DO` / `CB` | Comments mark signature and initializer text as final LLVM spelling / compatibility payloads; printer substitutes structured `LinkNameId` where present and otherwise outputs text. | Output boundary; only legacy scans make this producer-boundary compatibility, not semantic authority. | Not a name generator except embedded output spelling from lowering/constant-init emitters. | No for route-local identity cleanup; bridge-retirement lane should handle scans of final payloads. |
| BIR values and params | `src/backend/bir/bir.hpp` `Value::name`, `Param::name`; `bir_validate.cpp` `defined_names`; `bir_printer.cpp` `render_value` | `RL` | Header says `Value::name` is display/final spelling and not cross-object semantic identity; validation tracks params/results in `defined_names`; printer treats values as dump/final spelling. | Mostly one BIR block for def-before-use validation, with parameters seeding one BIR function; crosses BIR-to-prealloc through `ValueNameId`. | Usually inherited from LIR SSA names; materialized/generated names added in prealloc out-of-SSA. | Yes: raw value spellings synchronize BIR validation, liveness, regalloc, and prepared maps. |
| BIR block labels | `src/backend/bir/bir.hpp` `Block::label`, `PhiIncoming::label`, terminator labels, `BlockLabelId`; `bir_validate.cpp`; `lir_to_bir/module.cpp` `intern_known_block_labels` | `RL` / `CB` | Raw labels are compatibility spellings; `intern_known_block_labels` interns function block labels into `NameTables::block_labels` and rewrites phi/terminator label ids; validation enforces same-function targets by `BlockLabelId` when available, otherwise raw labels. | One BIR function; crosses LIR-to-BIR assignment and BIR-to-prealloc prepared label tables. | Mostly inherited from LIR labels; switch lowering generates `block.label + ".switch.case.<N>[.<dedupe>]"`. | Yes: block labels already have `BlockLabelId`, but raw fallback paths still coordinate local CFG structures. |
| BIR local slots and sret storage | `src/backend/bir/bir.hpp` `LocalSlot::name`, `LoadLocalInst::slot_name`, `StoreLocalInst::slot_name`, `CallInst::sret_storage_name`; `bir_validate.cpp` `find_local_slot` | `RL` | Header states slot handles are route-local within one lowered function; validation requires local load/store names to reference declared `Function::local_slots`; sret storage relates generated local slots. | One BIR function local storage namespace; crosses into prepared `SlotNameId` and stack-layout analysis. | Lowering-derived slot names plus prealloc phi storage `.phi` names. | Yes: local storage/provenance names should be typed local ids where multiple prealloc structures synchronize by spelling. |
| BIR memory address display bases | `src/backend/bir/bir.hpp` `MemoryAddress::base_name`, `base_link_name_id`, `base_label_id`; `bir_printer.cpp` `render_memory_address` | `RL` / `DO` / `CB` | Header says `base_name` is display/compatibility; global and label bases carry structured ids when known; printer uses names for dumps. | Address dump boundary, except local-slot/string-constant names stay in route-local/backend data namespaces. | Inherited from local slots, globals, labels, or string constants. | Yes only for local-slot/string-constant bases; no for pure display text. |
| BIR string constants | `src/backend/bir/bir.hpp` `StringConstant::name`; `src/backend/bir/lir_to_bir/globals.cpp` `lower_string_constant_global` | `GT` / `CB` | Header says string-pool constants are retained compatibility names and intentionally have no `LinkNameId`; lowering strips leading `@` and publishes an extern constant global. | Module-level generated private data namespace; crosses LIR-to-BIR and later backend addressing. | LIR `@.str<N>` generator, normalized to no leading `@` in BIR globals. | Yes: generated data labels are separate from source globals and need local/generated identity cleanup. |
| LIR-to-BIR route-local lowering maps | `src/backend/bir/lir_to_bir/lowering.hpp` `CompareMap`, `BlockLookup`, `AggregateValueAliasMap`; `module.cpp` selector/switch helpers | `RL` | Comments explicitly key compare/aggregate/block lookup maps by route-local SSA or raw LIR block spelling before BIR ids exist. | Single LIR-to-BIR function lowering pass before BIR `BlockLabelId` publication. | Derived from LIR SSA/block labels; switch case labels generated as above. | Yes: these are prime candidates for typed local ids or narrower map keys. |
| Prepared name tables | `src/backend/prealloc/prealloc.hpp` `PreparedNameTables`, `ValueNameId`, `BlockLabelId`, `SlotNameId`; `prepared_*_name`, `resolve_prepared_*` | `RL` | Prepared tables intern block, value, and slot spellings into backend-stage ids; helper names resolve ids back to spellings. | Prepared backend stage; ids are not source identity and are local to prepared module/function domains depending on table use. | Interned from BIR params/results/block labels/local slots and generated prealloc names. | Yes: this is the main route-local identity cleanup surface. |
| Prepared block/control-flow labels | `prealloc/out_of_ssa.cpp` `intern_preferred_block_label`, `find_block_by_prepared_label`, `build_parallel_copy_bundles`; `prealloc/liveness.cpp`/`legalize.cpp` similar intern helpers | `RL` / `CB` | Helpers prefer BIR `BlockLabelId` spelling and fall back to raw labels, then intern into prepared labels; parallel-copy and join-transfer state uses `BlockLabelId` edges. | One prepared function CFG; crosses BIR-to-prealloc, then within liveness/out-of-SSA/regalloc/legalize. | Interned from BIR labels; no new generator except derived continuation metadata. | Yes: fallback raw-label resolution and duplicated intern helpers should feed cleanup. |
| Prepared value/regalloc names | `prealloc/liveness.cpp` value interning, `prealloc/regalloc.cpp` `prepare_inst_result_value_name`, `find_regalloc_value`, pointer carrier maps, `append_*move_resolution` | `RL` / `CB` | Liveness/regalloc intern params and instruction results as `ValueNameId`; regalloc still finds by raw spelling in helper boundaries before resolving to ids. | One prepared function value namespace; crosses liveness, regalloc, value-location, storage-plan, and call-plan phases. | Inherited BIR values plus generated out-of-SSA select/materialization results. | Yes: raw spelling bridges around `ValueNameId` are central route-local cleanup candidates. |
| Prepared slot/stack names | `prealloc/stack_layout/analysis.cpp` `names.slot_names.intern(slot.name)`, `prealloc/out_of_ssa.cpp` `make_phi_slot_name`, `prealloc/stack_layout/**` `prepared_slot_name` lookups | `RL` / `GT` | Stack-layout analysis interns local slots; out-of-SSA creates phi storage names by appending `.phi`; stack-layout/coalescing/slot-assignment lookups use prepared slot names. | One prepared function stack/local storage namespace; crosses stack-layout and regalloc planning. | `make_phi_slot_name(result_name) -> result_name + ".phi"` plus inherited BIR local-slot names. | Yes: generated phi storage names and slot-name lookups should become typed local storage ids where possible. |
| Prepared generated select/materialization temps | `prealloc/out_of_ssa.cpp` `make_materialized_select_name`, `BlockAnalysis::defs_by_name`, `PhiMaterializeContext::materialized_names` | `GT` / `RL` | Generator appends `.phi.sel<N>` and checks both existing defs and materialized names for collision before emitting materialized values. | One BIR/prealloc function during out-of-SSA phi materialization. | `result_name + ".phi.sel" + next_temp_index++`, collision-checked against `defs_by_name` and `materialized_names`. | Yes: generated value names are local ids masquerading as strings. |
| Prepared local analysis maps | `prealloc/out_of_ssa.cpp` `BlockAnalysis::{blocks_by_label,defs_by_name}`, local `blocks_by_label`; `prealloc/regalloc.cpp` `last_loaded_pointer_name_by_slot`, `direct_step_by_value_name`, `pointer_carriers` | `RL` / `CB` | Analysis maps key CFG and defs by raw strings or local ids to synchronize transformations; regalloc maps use `ValueNameId` for pointer carriers but still has a slot-name string map. | One prepared function/pass-local analysis; no source/link boundary. | Map keys inherited from BIR/prepared names; generated temps as above. | Yes for raw-string maps that synchronize structures; lower priority for already-typed `ValueNameId` maps. |
| Register names and ABI storage strings | `prealloc/regalloc.cpp` `materialize_register_names`, `split_trailing_register_index`, call arg/result destination register helpers; `PreparedRegallocValue::assigned_register` | `DO` / `RL` | Register strings are target/backend storage spellings and final allocation/debug payload, not source semantic identity; occupied-register lists define local allocation resources. | Target-profile/register-bank local allocation domain; crosses into prepared printer/x86 emission. | Target profile register names and contiguous numeric expansion. | No for idea 169 route-local compiler identity, except keep separate from value identity in cleanup notes. |

## Suggested Next

Continue Step 3 with the second route-local/generated-name packet focused on
frontend and midend route-local/display names that were out of scope here:
parser/sema/HIR local labels, diagnostic/display-only names, compatibility
rendered maps, and any remaining final-output payloads that might be confused
with semantic authority.

## Watchouts

- `ValueNameId`, `BlockLabelId`, and `SlotNameId` are the right kind of local
  backend identity but still stage-local; do not promote them to `TextId` or
  `LinkNameId`.
- The strongest idea 169 candidates found here are raw-string maps that
  synchronize multiple local structures: LIR-to-BIR `BlockLookup`/`CompareMap`,
  BIR validation fallback label/slot/name lookup, prealloc `defs_by_name`,
  `blocks_by_label`, slot-name maps, and generated phi/select names.
- Final LLVM/BIR rendering remains output/diagnostic unless a path feeds the
  rendered text back into lookup; this packet found compatibility bridges but
  no new Step 3 `SA` source-semantic authority in these families.
- String-pool/private labels are generated addressable data names, not source
  string-literal text and not link-visible global authority.

## Proof

Classification/audit proof only; no tests were required because this packet
made no implementation or behavior changes and found no tiny behavior-sensitive
issue requiring execution proof. `test_after.log` was not updated.

Commands run:

`rg -n --glob '!build*/**' --glob '!ideas/closed/**' --glob '!review/**' -e 'ValueNameId' -e 'BlockLabelId' -e 'value_name' -e 'block_label' -e 'defs_by_name' -e 'blocks_by_label' -e 'slot' -e 'string_constant' -e 'string_pool' -e 'tmp' -e 'temporary' -e 'label' -e 'local' src/codegen/lir src/backend/bir src/backend/prealloc`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/prealloc.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/regalloc.cpp build/compile_commands.json`

`c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/prealloc/out_of_ssa.cpp build/compile_commands.json`

All four proof commands completed successfully. Targeted follow-up inspection
used `rg`/`sed` only and stayed within the packet's read scope.
