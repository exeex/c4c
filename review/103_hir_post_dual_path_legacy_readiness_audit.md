# HIR Post Dual-Path Legacy Readiness Audit

Status: In Progress
Source Idea: `ideas/open/103_hir_post_dual_path_legacy_readiness_audit.md`

## Step 1 - HIR Legacy Lookup APIs And Callers

Scope for this step: inventory remaining HIR legacy lookup APIs and direct
callers after ideas 99 through 102. This step did not change implementation
behavior.

### Lookup API Inventory

| API / Storage | Definition | Direct Call Sites Seen | Structured Identity Available | Provisional Classification | Notes |
|---|---|---|---|---|---|
| `Module::lookup_function_id(std::string_view)` via `fn_index` | `src/frontend/hir/hir_ir.hpp:1407` | `find_function_by_name_legacy` only in Step 1 scan | `FunctionId`; structured mirror is `fn_structured_index` keyed by `ModuleDeclLookupKey`; functions also carry `name_text_id` and `link_name_id` | `legacy-proof-only` for HIR-internal decl resolution; `bridge-required` where callers only have rendered names | Rendered-name index is still the backing store for `find_function_by_name_legacy`; structured decl lookup exists but not all callers carry a full `DeclRef`. |
| `Module::lookup_global_id(std::string_view)` via `global_index` | `src/frontend/hir/hir_ir.hpp:1412` | `find_global_by_name_legacy`; direct callers at `src/frontend/hir/impl/stmt/decl.cpp:18`, `src/frontend/hir/impl/templates/global.cpp:126`, `src/frontend/hir/impl/templates/global.cpp:137`, `src/frontend/hir/impl/expr/scalar_control.cpp:185` | `GlobalId`; structured mirror is `global_structured_index` keyed by `ModuleDeclLookupKey`; globals carry `name_text_id` and `link_name_id` | `bridge-required` | Direct callers mostly start from AST/rendered names or instantiated mangled names, not full `DeclRef` metadata. |
| `Module::find_function_by_name_legacy(std::string_view)` | `src/frontend/hir/hir_ir.hpp:1443` and `src/frontend/hir/hir_ir.hpp:1448` | HIR lowering: `hir_functions.cpp:220`, `hir_types.cpp:708`, `hir_types.cpp:720`, `hir_types.cpp:755`, `hir_types.cpp:2069`, `hir_types.cpp:2203`, `hir_types.cpp:2207`, `impl/expr/call.cpp:16`, `impl/expr/call.cpp:21`, `impl/expr/call.cpp:221`, `impl/expr/call.cpp:393`, `impl/expr/operator.cpp:187`, `impl/expr/operator.cpp:335`, `impl/expr/operator.cpp:462`, `impl/expr/builtin.cpp:141`, `impl/expr/scalar_control.cpp:230`, `impl/stmt/range_for.cpp:44`, `impl/stmt/range_for.cpp:153`, `impl/stmt/range_for.cpp:203`; HIR-to-LIR: `src/codegen/lir/hir_to_lir/call/target.cpp:23`, `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:552` | `FunctionId`, `LinkNameId`, `ModuleDeclLookupKey`, `name_text_id`; several call sites also have `DeclRef::link_name_id` | Mixed: `legacy-proof-only` inside `resolve_function_decl`; `bridge-required` / `blocked-by-hir-to-lir` for rendered-name-only lowering seams | AST-backed query on `impl/expr/call.cpp` found direct callers of this API through `find_direct_call_target` and `find_direct_call_carrier_link_name_id`. HIR-to-LIR still falls back from `LinkNameId` to rendered callee names. |
| `Module::find_function(LinkNameId)` | `src/frontend/hir/hir_ir.hpp:1429` and `src/frontend/hir/hir_ir.hpp:1436` | `classify_function_decl_lookup`; `impl/expr/call.cpp` direct-call helpers; HIR-to-LIR call target and const-init paths | `LinkNameId` | `safe-to-keep` as structured/link lookup | This is not legacy rendered lookup; remaining risk is fallback to `find_function_by_name_legacy` when `LinkNameId` is invalid. |
| `Module::find_function_by_decl_ref_structured(const DeclRef&)` | `src/frontend/hir/hir_ir.hpp:1477` and `src/frontend/hir/hir_ir.hpp:1484` | `classify_function_decl_lookup`, `resolve_function_decl` | `DeclRef::name_text_id`, namespace `TextId`s, `ModuleDeclLookupKey`, `FunctionId` | `safe-to-demote` target for HIR-internal decl lookup | Structured lookup is already authoritative when it hits, except `resolve_function_decl` intentionally returns legacy on mismatch for parity visibility. |
| `Module::classify_function_decl_lookup` / `resolve_function_decl` | `src/frontend/hir/hir_ir.hpp:1497` / `src/frontend/hir/hir_ir.hpp:1525` | dump/inspection at `src/frontend/hir/impl/inspect/printer.cpp:557`; HIR decl resolution callers via `resolve_function_decl` | `LinkNameId`, `ModuleDeclLookupKey`, fallback rendered name | `legacy-proof-only` | These functions record lookup authority and parity mismatches. Keep during proof; eventual demotion should remove legacy authority after parity proof, not before. |
| `Module::find_global_by_name_legacy(std::string_view)` | `src/frontend/hir/hir_ir.hpp:1566` and `src/frontend/hir/hir_ir.hpp:1571` | `hir_types.cpp:705`, `hir_types.cpp:2066` | `GlobalId`, `LinkNameId`, `ModuleDeclLookupKey`, `name_text_id` | `bridge-required` | Direct use remains in type inference from rendered AST names. Structured lookup exists but those call sites do not currently pass complete `DeclRef` metadata. |
| `Module::find_global(LinkNameId)` | `src/frontend/hir/hir_ir.hpp:1552` and `src/frontend/hir/hir_ir.hpp:1559` | `classify_global_decl_lookup`; HIR-to-LIR const-init `select_global_object(const DeclRef&)` | `LinkNameId` | `safe-to-keep` as structured/link lookup | Not legacy; fallback pressure remains where link id is invalid and code selects by rendered name. |
| `Module::find_global_by_decl_ref_structured(const DeclRef&)` | `src/frontend/hir/hir_ir.hpp:1576` and `src/frontend/hir/hir_ir.hpp:1583` | `classify_global_decl_lookup`, `resolve_global_decl` | `DeclRef::name_text_id`, namespace `TextId`s, `ModuleDeclLookupKey`, `GlobalId` | `safe-to-demote` target for HIR-internal decl lookup | Same shape as function structured lookup; legacy is still retained for mismatch observation. |
| `Module::classify_global_decl_lookup` / `resolve_global_decl` | `src/frontend/hir/hir_ir.hpp:1596` / `src/frontend/hir/hir_ir.hpp:1631` | dump/inspection at `src/frontend/hir/impl/inspect/printer.cpp:553`; HIR decl resolution callers via `resolve_global_decl` | `GlobalId`, `LinkNameId`, `ModuleDeclLookupKey`, fallback rendered name | `legacy-proof-only` | Concrete `GlobalId` and `LinkNameId` are already preferred before structured/rendered lookup. |
| `CompileTimeState::find_template_def(const std::string&)` and `has_template_def(const std::string&)` | `src/frontend/hir/compile_time_engine.hpp:973`, `src/frontend/hir/compile_time_engine.hpp:1026` | `hir_build.cpp:191`, `:208`, `:217`, `:233`, `:249`, `:284`, `:334`, `:627`; `hir_types.cpp:733`; `impl/expr/call.cpp:225`, `:408`, `:459` | Structured overload exists: `find_template_def(const Node*, rendered_name)` using `CompileTimeRegistryKey`; instances use `SpecializationKey` | `bridge-required` | Many callers only have `Node::name`; the structured overload is available but not the dominant call shape. |
| `CompileTimeState::find_consteval_def(const std::string&)` and `has_consteval_def(const std::string&)` | `src/frontend/hir/compile_time_engine.hpp:999`, `src/frontend/hir/compile_time_engine.hpp:1083` | `impl/compile_time/engine.cpp:150`, `:364`, `:412`; `impl/expr/call.cpp:403`, `:565`; `hir_types.cpp:728`; `impl/expr/scalar_control.cpp:293` | Structured overload exists: `find_consteval_def(const Node*, rendered_name)` using `CompileTimeRegistryKey` | `bridge-required` | Engine pending consteval records carry `fn_name` strings, so string lookup is still authoritative in those paths. |
| `InstantiationRegistry::find_instances(const std::string&)` | `src/frontend/hir/compile_time_engine.hpp:563` | `hir_build.cpp:252`, `:337`, `:543`; `hir_types.cpp:747`; `impl/expr/call.cpp:426`; `compile_time_engine.hpp:1177` | Instances carry `SpecializationKey` and optional `primary_def`; registry also keeps structured seed/instance keys | `needs-more-parity-proof` | Callers usually enter by template function name, then compare `SpecializationKey`. Demotion needs an owner-key lookup surface for callers that have `primary_def`. |
| `CompileTimeState::find_template_struct_def(const std::string&)` and specialization lookup by name | `src/frontend/hir/compile_time_engine.hpp:1041`, `src/frontend/hir/compile_time_engine.hpp:1057` | `compile_time_engine.hpp:1252`; lowerer wraps these through `find_template_struct_primary` / `find_template_struct_specializations` | Structured overloads exist with `CompileTimeRegistryKey`; lowerer also mirrors `HirRecordOwnerKey` | `needs-more-parity-proof` | Both compile-time-state and lowerer maps now have structured mirrors, but lookup callers still begin with rendered primary names in several routes. |
| `Lowerer::find_template_struct_primary(const std::string&)` | `src/frontend/hir/impl/templates/templates.cpp:436` | `hir_build.cpp:149`, `:158`; `hir_types.cpp:2026`, `:2032`, `:2186`; `impl/expr/call.cpp:61`; `impl/expr/scalar_control.cpp:50`, `:56`, `:103`, `:109`; `impl/templates/templates.cpp:542`, `:557`, `:567`, `:572`, `:641`; `impl/templates/struct_instantiation.cpp:46`; `impl/templates/type_resolution.cpp:280`, `:284`, `:376`; `impl/templates/value_args.cpp:371` | `HirRecordOwnerKey` mirror in `template_struct_defs_by_owner_`; `TextId` derived from AST declaration | `legacy-proof-only` today, not deletion-ready | Function records parity against the owner-key map but returns the rendered lookup result. |
| `Lowerer::find_template_struct_specializations(const Node*)` | `src/frontend/hir/impl/templates/templates.cpp:444` | `build_template_struct_env` at `src/frontend/hir/impl/templates/templates.cpp:508` | `HirRecordOwnerKey`; `CompileTimeRegistryKey` through `ct_state_` | `legacy-proof-only` | It first asks `ct_state_` by primary definition plus rendered fallback, then falls back to lowerer rendered map. Returned value remains the rendered/specialization vector used by lowering. |
| `Lowerer::enum_consts_` and `CompileTimeState::enum_consts()` | `src/frontend/hir/impl/lowerer.hpp:1011`, `src/frontend/hir/compile_time_engine.hpp:1374` | `hir_build.cpp:106`; `hir_types.cpp:180`, `:182`, `:1588`; `impl/expr/scalar_control.cpp:138`; env construction in `hir_types.cpp:86` and `impl/compile_time/engine.cpp:79` | `CompileTimeValueBindingKey` structured mirrors: `enum_consts_by_key_`; env exposes `enum_consts_by_key` | `bridge-required` | Structured maps are copied into consteval env, but legacy maps are still direct evaluation inputs and block-scoped save/restore state. |
| `Lowerer::const_int_bindings_` and `CompileTimeState::const_int_bindings()` | `src/frontend/hir/impl/lowerer.hpp:1012`, `src/frontend/hir/compile_time_engine.hpp:1379` | `hir_types.cpp:1961`, `:1968`; env construction in `hir_types.cpp:87` and `impl/compile_time/engine.cpp:79` | `CompileTimeValueBindingKey` structured mirrors: `const_int_bindings_by_key_`; env exposes `named_consts_by_key` | `bridge-required` | Global const-int binding still reaches evaluators as a name map; structured mirrors are present but not the only authority. |
| `Module::struct_defs` rendered tag map | `src/frontend/hir/hir_ir.hpp:1252` | Broad direct consumers include `hir_types.cpp:96`, `:548`, `:584`, `:620`, `:795`, `:871`, `:892`, `:1025`, `:1118`, `:1157`, `:1225`, `:1249`, `:1267`; `impl/expr/*`, `impl/stmt/*`, HIR-to-LIR/shared codegen will be covered in later steps | Structured owner index exists: `struct_def_owner_index` keyed by `HirRecordOwnerKey`, plus `find_struct_def_by_owner_structured` | `bridge-required` | Step 1 confirms owner lookup exists but rendered `TypeSpec::tag` and struct maps still drive many semantic and codegen paths. |
| `Module::find_struct_def_by_owner_structured` / owner helpers | `src/frontend/hir/hir_ir.hpp:1349`, `:1356`, `:1364` | Currently mostly parity/owner registration surface; no broad direct semantic consumers found in Step 1 scan | `HirRecordOwnerKey` | `safe-to-demote` target, but `needs-more-parity-proof` for replacing `struct_defs.find(tag)` | This is the structured target for idea 104, but rendered tag consumers are too broad to delete yet. |
| `Lowerer::find_struct_method_mangled` | `src/frontend/hir/hir_types.cpp:524` | `impl/expr/call.cpp:72`, `:76`, `:290`, `:490`, `:493`; `impl/expr/operator.cpp:134`; recursive base lookup in `hir_types.cpp:552` | `HirStructMethodLookupKey` built from `HirRecordOwnerKey`, method `TextId`, constness; rendered value is mangled name | `bridge-required` | Helper records parity against `struct_methods_by_owner_` but returns rendered lookup. Direct map probes in `impl/stmt/range_for.cpp` bypass this helper. |
| `Lowerer::find_struct_method_link_name_id` | `src/frontend/hir/hir_types.cpp:560` | `impl/expr/call.cpp:74`, `:78`, `:497`; recursive base lookup in `hir_types.cpp:588` | `HirStructMethodLookupKey`; `LinkNameId` | `needs-more-parity-proof` | Structured `LinkNameId` mirror exists and should become preferred before deleting rendered fallback. |
| `Lowerer::find_struct_method_return_type` | `src/frontend/hir/hir_types.cpp:596` | `impl/expr/call.cpp:113`, `:2212`; `impl/expr/operator.cpp:192`; recursive base lookup in `hir_types.cpp:624` | `HirStructMethodLookupKey`; return `TypeSpec` | `bridge-required` | Return-type lookup still depends on rendered owner tags and direct rendered-map probes in range-for and generic control inference. |
| Direct `struct_methods_` / `struct_method_ret_types_` probes | maps in `src/frontend/hir/impl/lowerer.hpp:1059` and `:1070` | `impl/stmt/range_for.cpp:23`, `:49`, `:110`, `:140`, `:191`, `:208`; `hir_types.cpp:2092`; ref-overload code also probes rendered method keys | Owner-key mirrors exist for method, link-name, and return-type maps | `authoritative string lookup` / `bridge-required` | These bypass the parity-recording helpers and are currently the highest-risk method cleanup blockers. |
| `Lowerer::find_struct_static_member_decl` | `src/frontend/hir/hir_types.cpp:1215` | `impl/expr/operator.cpp:123`; `impl/expr/scalar_control.cpp:95`, `:102`, `:123`, `:129`, `:192`; recursive base lookup in `hir_types.cpp:1228` | `HirStructMemberLookupKey` from owner key plus member `TextId`; structured decl mirror | `legacy-proof-only` with bridge callers | Helper records parity but still returns rendered lookup and recursively follows rendered base tags. |
| `Lowerer::find_struct_static_member_const_value` | `src/frontend/hir/hir_types.cpp:1235` | `impl/expr/operator.cpp:121`; `impl/expr/scalar_control.cpp:93`, `:121`, `:190`; recursive base lookup in `hir_types.cpp:1252` | `HirStructMemberLookupKey`; structured const-value mirror | `bridge-required` | Also falls through to instantiated trait/static member evaluation, so deletion requires separating lookup identity from trait evaluation. |
| `Lowerer::find_struct_member_symbol_id` | `src/frontend/hir/hir_types.cpp:1259` | `impl/expr/operator.cpp:388`, `:395`, `:401`, `:423`, `:430`, `:436`; `impl/expr/object.cpp:312`; `impl/expr/scalar_control.cpp:217`; `impl/stmt/stmt.cpp:759`; recursive base lookup in `hir_types.cpp:1271` | `MemberSymbolId`; `HirStructMemberLookupKey`; field `TextId` | `needs-more-parity-proof` | The direct lookup still constructs rendered `tag + "::" + member` for `member_symbols.find`. Structured mirror exists but is parity-only. |
| `Lowerer::resolve_member_lookup_owner_tag` | `src/frontend/hir/impl/templates/value_args.cpp:208` | `impl/expr/operator.cpp:384`, `:391`, `:396`, `:419`, `:426`, `:431` | Inputs may include `TypeSpec`, template bindings, NTTP bindings; output is rendered owner tag | `bridge-required` | This is a bridge from richer type/template context back to rendered owner tags before member-symbol lookup. |

### Fallback-Only vs Authoritative String Lookup

- Fallback-only or parity-observation paths: `resolve_function_decl`,
  `resolve_global_decl`, `classify_*_decl_lookup`, and the `record_*_parity`
  helpers. These already prefer concrete IDs or structured keys where they can,
  then keep legacy strings to detect mismatches.
- Authoritative rendered lookup still exists in direct `struct_methods_`,
  `struct_method_ret_types_`, `struct_defs`, `enum_consts_`,
  `const_int_bindings_`, template name registries, and `find_*_by_name_legacy`
  callers that start from an AST `Node::name`, `TypeSpec::tag`, or a synthesized
  mangled name.
- Downstream HIR-to-LIR fallback is still present for function calls and const
  initializers: `StmtEmitter::find_local_target_function` and
  `ConstInitEmitter::emit_const_scalar_expr` both prefer `LinkNameId` where
  present, then fall back to rendered names.

### Lightweight Checks Run

- `command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb`
- `test -f build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures` on `hir_build.cpp`,
  `impl/templates/templates.cpp`, and `impl/expr/call.cpp`
- `c4c-clang-tool-ccdb function-callers` on targeted HIR and HIR-to-LIR
  translation units for legacy module and struct lookup helpers
- Focused `rg` scans for `find_*_legacy`, template/consteval registry calls,
  enum/const-int maps, struct owner/member/method helpers, and direct rendered
  method-map probes

## Step 2 - String-Keyed HIR Maps And Fallback Classification

Scope for this step: classify the remaining HIR string-keyed storage, rendered
fallbacks, and structured mirrors from Step 1 without changing implementation
behavior.

### Classification Summary

| Map / Fallback Family | Primary Classification | Justification | Follow-Up Boundary |
|---|---|---|---|
| `Module::fn_structured_index`, `global_structured_index`, `find_*_by_decl_ref_structured` | `safe-to-demote` target | These are keyed by `ModuleDeclLookupKey` and can resolve from `DeclRef` metadata when `name_text_id` and namespace `TextId`s are complete. They are the intended replacement authority for HIR-internal decl lookup. | Idea 104 can prefer these only at call sites that already carry complete `DeclRef` metadata. |
| `Module::classify_*_decl_lookup`, `resolve_*_decl`, and `decl_lookup_*` parity records | `legacy-proof-only` | These surfaces compare structured and rendered lookup, record authority/mismatches, and intentionally return legacy on mismatch to preserve existing behavior during proof. | Keep until parity proof says legacy authority can be removed; do not treat mismatch-preserving behavior as cleanup-ready code. |
| `Module::fn_index` and `global_index` backing `find_*_by_name_legacy` | `bridge-required` | The maps still serve callers that start from `Node::name`, synthesized mangled names, or downstream `DeclRef::name` fallback rather than a full structured key. | Split HIR-internal callers from HIR-to-LIR and AST-name callers before demoting. |
| `CompileTimeState::template_fn_defs_` / `consteval_fn_defs_` rendered maps | `bridge-required` | Structured overloads exist, but pending consteval and several template-call routes still carry function names as strings. | Add owner/declaration-key lookup at the call boundaries before deleting name lookups. |
| `CompileTimeState::template_struct_defs_` and `template_struct_specializations_` | `needs-more-parity-proof` | Structured key maps exist and lowerer parity counters compare them, but `find_template_struct_def(std::string)` and specialization lookup by primary name still feed active template-struct resolution. | Prove owner-key lookup covers primary and specialization routes before demotion. |
| `InstantiationRegistry::seed_work_` / `instances_` keyed by function name | `needs-more-parity-proof` | The registry has structured seed/instance dedup sets and owner-based specialization maps, but realized instance lookup still enters by rendered template function name and then checks `SpecializationKey`. | Introduce or prove an owner-key instance lookup for callers with `primary_def`. |
| Lowerer `template_struct_defs_`, `template_struct_specializations_`, and owner mirrors | `legacy-proof-only` today; `needs-more-parity-proof` for deletion | `find_template_struct_primary` returns rendered-map results and records parity against `template_struct_defs_by_owner_`; specialization lookup consults compile-time structured lookup but can still fall back to rendered primary names. | Keep parity reporting; do not delete rendered storage until lookup authority flips and broad template-struct cases are proved. |
| Lowerer `enum_consts_` / `const_int_bindings_` and `CompileTimeState` equivalents | `bridge-required` | `CompileTimeValueBindingKey` mirrors are copied into consteval environments, but legacy name maps still feed `static_eval_int`, block-scoped save/restore, and evaluator compatibility inputs. | Convert evaluator/env APIs to structured bindings before demotion. |
| `Module::struct_def_owner_index` and `find_struct_def_by_owner_structured` | `safe-to-demote` target | Owner-key lookup is structured and records owner/rendered parity mismatches. It is suitable as the replacement surface where callers have `HirRecordOwnerKey`. | Idea 104 should use it for owner-aware HIR paths, not for raw `TypeSpec::tag` consumers. |
| `Module::struct_defs` rendered tag map | `bridge-required` | It remains the broad semantic storage for `TypeSpec::tag`, base traversal, layout, field lookup, initialization normalization, and HIR-to-LIR struct layout selection. | Requires a tag-to-owner bridge or structured owner handoff before rendered tag lookup can be demoted. |
| Lowerer struct method rendered maps: `struct_methods_`, `struct_method_link_name_ids_`, `struct_method_ret_types_` | `bridge-required` | Helper APIs record parity against owner-key mirrors, but return rendered lookup results and recurse through `module_->struct_defs` base tags. Direct range-for/object/operator probes bypass some helpers. | First route direct probes through structured-aware helpers, then flip helpers to structured authority after parity proof. |
| Lowerer struct method owner mirrors and parity counters | `legacy-proof-only` | `struct_methods_by_owner_`, `struct_method_link_name_ids_by_owner_`, and `struct_method_ret_types_by_owner_` are currently comparison mirrors for rendered-map results. | Keep as proof infrastructure until lookup authority is flipped. |
| Lowerer static-member rendered maps: `struct_static_member_decls_`, `struct_static_member_const_values_` | `bridge-required` | Helpers record owner-key parity but still search rendered owning tags, then recurse over rendered base tags; const-value lookup also includes instantiated trait/static-member evaluation. | Separate member identity lookup from trait/static evaluation before demotion. |
| Lowerer static-member owner mirrors and parity counters | `legacy-proof-only` | `struct_static_member_*_by_owner_` maps currently validate rendered results rather than drive lookup. | Retain until owner-key lookup becomes authoritative and base traversal is owner-aware. |
| `struct_member_symbol_ids_by_owner_` mirror versus `module_->member_symbols.find(tag + "::" + member)` | `needs-more-parity-proof` | A structured mirror exists, but `find_struct_member_symbol_id` still constructs rendered member symbols and recurses through rendered base tags. | Prove owner-key member-symbol lookup and inherited lookup semantics before demotion. |
| HIR-to-LIR `find_local_target_function` fallback to `find_function_by_name_legacy` | `bridge-required` | `LinkNameId` is preferred, but fallback names are still used for call targets when link identity is missing. | Idea 105 should carry structured/link identity across the HIR-to-LIR seam. |
| HIR-to-LIR `select_global_object(std::string)` and `DeclRef::name` fallback | `bridge-required` | `GlobalId` and `LinkNameId` overloads exist, but both `StmtEmitter` and `ConstInitEmitter` fall back to rendered names and also choose a best global variant by name. | Downstream bridge work must preserve best-object selection semantics while replacing name-only identity. |
| HIR-to-LIR direct `mod.struct_defs.find(TypeSpec::tag)` consumers | `bridge-required` | Struct layout, varargs, const-init, expression, and type lowering still consume rendered tags from HIR payloads. | Keep out of HIR-only demotion; this is downstream bridge scope. |

### Safe Demotion Candidates

- Structured module decl lookup by `ModuleDeclLookupKey` is safe as the target
  for HIR-internal call sites that already carry complete `DeclRef` metadata.
- Structured record owner lookup by `HirRecordOwnerKey` is safe as the target
  for owner-aware HIR paths.
- Link-name and concrete-ID lookup (`FunctionId`, `GlobalId`, `LinkNameId`) is
  already structured enough and should stay as preferred authority where
  available.

### Not Yet Demotion-Ready

- Rendered maps that still receive `Node::name`, `TypeSpec::tag`, synthesized
  mangled names, or downstream `DeclRef::name` are `bridge-required`.
- Mirrors that only count or record parity are `legacy-proof-only`; deleting
  the rendered side before flipping authority would remove the proof surface.
- Template instance and struct-member symbol lookup need more parity proof
  because structured mirrors exist but lookup still depends on rendered owner
  or primary names.

### Additional Lightweight Checks Run For Step 2

- `rg` over `src/frontend/hir` and `src/codegen/lir/hir_to_lir` for rendered
  HIR maps, `_by_owner_`, `_by_key_`, `find_*_legacy`, `select_global_object`,
  `find_local_target_function`, and direct `struct_defs.find` consumers.
- Read-only inspection of `src/frontend/hir/hir_ir.hpp`,
  `src/frontend/hir/compile_time_engine.hpp`,
  `src/frontend/hir/impl/lowerer.hpp`, `src/frontend/hir/hir_types.cpp`,
  `src/frontend/hir/impl/templates/templates.cpp`,
  `src/codegen/lir/hir_to_lir/core.cpp`, and
  `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`.
- `c4c-clang-tool-ccdb function-callers` was attempted for targeted helper
  symbols; the direct caller command reported no same-TU callers for those
  method-qualified/cross-TU paths, so classification used AST availability
  checks plus focused source scans.

## Step 3 - Diagnostics, Dumps, ABI, And Printer Spelling

Scope for this step: classify HIR textual surfaces separately from semantic
lookup authority. This step did not change implementation behavior.

### Textual Surface Classification

| Surface Family | Classification | Semantic Lookup Authority? | Evidence / Boundary |
|---|---|---|---|
| HIR printer type spelling via `ts_str` | `printer-only` | No | `src/frontend/hir/impl/inspect/printer.cpp:49` formats `TypeSpec` for dumps, including `struct/union/enum` `ts.tag` text at lines `71`-`73`. This is display output and should not block demoting lookup authority. |
| HIR module dump section ordering and struct printing | `dump` / `printer-only` | No, except it reads existing rendered order | `Printer::run` iterates `struct_def_order` and reads `m_.struct_defs.find(tag)` only to print known structs at `src/frontend/hir/impl/inspect/printer.cpp:194`-`199`; `print_struct_def` emits `sd.tag`, layout, and field text at lines `301`-`323`. This is dump presentation over current storage, not an independent lookup contract. |
| HIR decl lookup hit/parity dump text | `dump` / `legacy-proof-only` | No new authority | The printer renders recorded lookup hits and mismatches at `src/frontend/hir/impl/inspect/printer.cpp:174`-`191`; `DeclRef` inline printing calls `classify_*_decl_lookup` for qualified refs at lines `551`-`560`. The authority is the classifier result from `Module`, not the printer. Keep as proof/debug output until parity records are retired. |
| Template, consteval, and pending-consteval HIR print text | `printer-only` | No | Template instances print `inst.mangled_name`, `spec_key`, and type bindings at `src/frontend/hir/impl/inspect/printer.cpp:248`-`279`; consteval and pending consteval print `fn_name` and `ts_str` bindings at lines `281`-`298` and `664`-`680`. These are inspectable names only. |
| Compile-time diagnostics stored in `CompileTimeDiagnostic` and `Module::ct_info.diagnostics` | `diagnostic-only` | No | Diagnostic records carry human-readable descriptions at `src/frontend/hir/compile_time_engine.hpp:1390`-`1420`; `build_hir` converts them to `file:line:column: error:` strings at `src/frontend/hir/hir.cpp:34`-`43`. These strings report irreducible work and should not be treated as lookup keys. |
| Compile-time diagnostic descriptions using template / consteval names | `diagnostic-only` | No | The engine builds messages such as unresolved template calls, unreduced consteval calls, and static assert failures at `src/frontend/hir/impl/compile_time/engine.cpp:124`-`139`, `203`-`229`, `320`-`335`, and `558`-`566`, then appends them to debug stats at lines `750`-`753`. The underlying work queues still need separate lookup classification from Step 2. |
| Compile-time debug dumps and registry parity dumps | `dump` / `legacy-proof-only` | No new authority | `InstantiationRegistry::dump_parity` prints seed/instance counts and per-function detail at `src/frontend/hir/compile_time_engine.hpp:752`-`765`; `CompileTimeState::dump` prints rendered and structured registry sizes at lines `1292`-`1310`. These are proof visibility, not demotion blockers by themselves. |
| `canonical_type_str`, `type_suffix_for_mangling`, and pending type refs | `TypeSpec::tag textual surface`; mixed display/ABI input | Not semantic lookup by itself | `canonical_type_str` uses `struct.` / `union.` / `enum.` plus `ts.tag` at `src/frontend/hir/hir_ir.hpp:1121`-`1123`; `type_suffix_for_mangling` and `encode_pending_type_ref` use `ts.tag` at `src/frontend/hir/compile_time_engine.hpp:381` and `493`-`509`. These are textual fingerprints. Demotion requires preserving stable spellings or replacing them with structured canonical keys before treating them as removable strings. |
| Range-for and call lowering error messages containing rendered tags or names | `diagnostic-only`, but adjacent to semantic lookup | No, but do not hide adjacent lookups | Range-for messages embed `range_ts.tag` / `iter_ts.tag` at `src/frontend/hir/impl/stmt/range_for.cpp:30`-`33`, `113`-`115`, `143`-`145`, and `193`-`196`; template and consteval call errors embed source names at `src/frontend/hir/impl/expr/call.cpp:471`-`476` and `628`-`632`. The messages are diagnostic-only, while the surrounding rendered method/function lookups remain Step 2 bridge-required. |
| HIR-to-LIR runtime/lowering error text | `diagnostic-only` | No | Member-lvalue errors include `m.field` and resolved `access.tag` at `src/codegen/lir/hir_to_lir/lvalue.cpp:309`-`315`. They report failed lowering after lookup attempts and are not themselves lookup authority. |
| HIR-to-LIR emitted link spelling helper | `ABI/link-spelling` | Yes for emitted symbol spelling, not semantic lookup | `emitted_link_name` prefers `LinkNameId` spelling and falls back to raw names in `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:27`-`31` and `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:43`-`47`. This is the ABI-visible spelling surface; it must retain exact output while semantic lookup migrates away from rendered names. |
| LIR global/function/extern printer link-name resolution | `ABI/link-spelling` / `printer-only final emission` | Uses link IDs when present | `src/codegen/lir/lir_printer.cpp:17`-`67` resolves link names for signatures, direct calls, globals, and extern decls; final printing uses those names at lines `515`-`517`, `545`-`548`, `555`-`557`, and metadata lines `563`-`569`. This is output spelling and dedup rendering, not HIR semantic lookup authority. |
| HIR-to-LIR extern declaration dedup and filtering | `ABI/link-spelling` with fallback bridge | Partly semantic at the link boundary | `finalize_module` filters extern decls by `LinkNameId` first and falls back to `fn_index` by rendered name at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:394`-`423`. Keep classified apart from HIR-internal lookup: it is downstream link-boundary behavior and remains bridge-required for idea 105. |
| LLVM struct type declarations and `TypeSpec::tag`-derived LLVM types | `TypeSpec::tag textual surface` / `ABI/layout spelling` | Yes for layout/type emission until bridged | `build_type_decls` walks `struct_def_order`, renders `%struct` type names from tags, and follows `base_tags` through `mod.struct_defs` at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:235`-`275`. This is not just display text: it is emitted IR/layout spelling and remains bridge-required outside HIR-only cleanup. |
| HIR-to-LIR `TypeSpec::tag` layout and field consumers | `TypeSpec::tag textual surface` / semantic codegen input | Yes until owner/layout bridge exists | Object alignment and globals use `mod.struct_defs.find(ts.tag)` at `src/codegen/lir/hir_to_lir/hir_to_lir.cpp:54`-`56` and `151`-`155`; const-init and member lowering use tag lookups at `src/codegen/lir/hir_to_lir/const_init_emitter.cpp:1271`-`1272` and `src/codegen/lir/hir_to_lir/lvalue.cpp:601`-`616`. These are downstream semantic consumers and remain bridge-required. |
| `HirRecordOwnerKey` comments and owner-to-rendered tag bridge | `dump/codegen compatibility note` plus structured target | Structured target is separate from spelling | `src/frontend/hir/hir_ir.hpp:890`-`891` explicitly says owner keys identify records without using rendered tag spelling that codegen and dumps still consume. `index_struct_def_owner` stores owner-to-rendered mappings at lines `1338`-`1347`. This supports separating structured authority from textual tag compatibility. |

### Classification Boundary

- `diagnostic-only` and `printer-only` surfaces may keep rendered names for
  readability while semantic lookup authority is demoted elsewhere.
- `dump` surfaces that expose parity records are proof tooling, not lookup
  authority. Keep them until the matching proof infrastructure is retired.
- `ABI/link-spelling` surfaces must preserve emitted symbol spelling even after
  lookup authority moves to `LinkNameId`, concrete IDs, or structured keys.
- `TypeSpec::tag` is not one class of work: HIR printer and diagnostic uses are
  textual, while HIR-to-LIR layout/type emission and `mod.struct_defs.find(tag)`
  remain semantic/codegen bridge work.

### Lightweight Checks Run For Step 3

- Focused `rg` scans over `src/frontend/hir`, `src/codegen/lir/hir_to_lir`,
  and `src/codegen/lir` for diagnostics, dumps, printer paths, `TypeSpec::tag`,
  link-name spelling, ABI-visible symbol rendering, and `struct_defs.find`
  consumers.
- Read-only inspection of `src/frontend/hir/impl/inspect/printer.cpp`,
  `src/frontend/hir/impl/compile_time/engine.cpp`,
  `src/frontend/hir/compile_time_engine.hpp`, `src/frontend/hir/hir.cpp`,
  `src/frontend/hir/hir_ir.hpp`, `src/frontend/hir/impl/stmt/range_for.cpp`,
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`,
  `src/codegen/lir/hir_to_lir/const_init_emitter.cpp`,
  `src/codegen/lir/hir_to_lir/lvalue.cpp`, and
  `src/codegen/lir/lir_printer.cpp`.
- No `c4c-clang-tool` query was needed for this packet because the owned work
  classified textual surfaces from concrete print/diagnostic/render call sites.
