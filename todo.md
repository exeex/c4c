# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory HIR Rendered Lookup Surfaces

## Just Finished

Step 1 - Inventory HIR Rendered Lookup Surfaces completed the HIR string
surface classification for `src/frontend/hir`.

Classified surface map:

- Semantic authority still present: `NttpBindings` is
  `std::unordered_map<std::string, long long>` in
  `src/frontend/hir/hir_ir.hpp`, flows through `FunctionCtx`,
  `TemplateCallInfo`, `HirTemplateInstantiation`, compile-time state, template
  materialization, consteval call setup, and direct `ctx->nttp_bindings.find(...)`
  probes. Sema already has `ConstEvalEnv::nttp_bindings_by_text` and
  `nttp_bindings_by_key`, so this is the first metadata-backed migration
  family rather than display text.
- Semantic authority with structured alternatives already present:
  `Module::fn_index` / `global_index`, `find_function_by_name_legacy`,
  `find_global_by_name_legacy`, and
  `ModuleDeclLookupAuthority::LegacyRendered` still provide rendered-name
  fallback for declaration lookup, while `FunctionId`, `GlobalId`,
  `LinkNameId`, and `ModuleDeclLookupKey` are already available and are
  preferred when populated.
- Semantic authority with structured alternatives already present:
  `Lowerer` struct method/member maps keyed by rendered
  `tag::member` strings (`struct_methods_`,
  `struct_method_link_name_ids_`, `struct_method_ret_types_`,
  `struct_static_member_decls_`, `struct_static_member_const_values_`, and
  `member_symbols.find(tag + "::" + member)`) have owner/member mirrors keyed
  by `HirStructMethodLookupKey` or `HirStructMemberLookupKey`. Current helpers
  are structured-primary when owner keys exist, but rendered fallback remains
  semantic compatibility.
- Semantic authority with structured alternatives already present:
  `CompileTimeState` template/consteval registries keep rendered maps
  (`template_fn_defs_`, `template_struct_defs_`,
  `template_struct_specializations_`, `consteval_fn_defs_`) beside
  `CompileTimeRegistryKey` mirrors. Lookup helpers prefer declaration identity
  when supplied and use rendered names as explicit fallback.
- Compatibility/no-metadata fallback: `fallback_tag` and `fallback_member` in
  `find_struct_static_member_const_value(HirStructMemberLookupKey, ...)`
  recover rendered owner/member spelling when the structured value map misses.
  This is not display text; it is a compatibility semantic fallback and should
  stay narrow until the value map is reliably populated.
- Compatibility/no-metadata fallback: `has_legacy_mangled_entry` and
  `InstantiationRegistry` rendered seed/instance maps still use mangled text
  when `FunctionTemplateInstanceKey` lacks a declaration-backed primary.
  Structured seed/instance keys exist and should remain authoritative where
  `primary_def` is present.
- Final spelling/mangling/link text: `LinkNameId`, `link_names`,
  `mangle_template_name`, `HirTemplateInstantiation::mangled_name`, generated
  method names, weak symbol strings, and emitted function/global names are
  payloads for ABI/link-visible spelling or deterministic output, not lookup
  authority by themselves.
- Diagnostics/dumps: `format_hir`, compile-time/materialization stats,
  pending-template diagnostics, parity mismatch records, and debug labels keep
  strings for human-visible reporting.
- Local scratch: transient `std::string` construction for temporary local
  names, label names, qualified-name splitting, pack-binding names,
  `context_name`, and parser-derived `TypeSpec::tag` rendering is local
  lowering scratch unless it is used to probe one of the semantic maps above.
- Unresolved metadata boundary: Sema `lookup_record_layout` still receives
  rendered-keyed `ConstEvalEnv::struct_defs` while HIR already owns
  `Module::struct_def_owner_index` and
  `find_struct_def_by_owner_structured(...)`; this requires a structured HIR
  record-layout handoff in a later step.
- Unresolved metadata boundary: Lowerer `function_decl_nodes_` is an early
  template-deduction return-type probe keyed by unqualified strings before
  ordinary functions are fully lowered into the module.

First concrete migration target: migrate metadata-backed HIR NTTP/consteval
handoff from string-keyed `NttpBindings` toward a `TextId`/structured-key
carrier, starting at `build_call_nttp_bindings`, `FunctionCtx::nttp_bindings`,
`TemplateCallInfo::nttp_args`, and the `ConstEvalEnv` setup sites that
currently assign only `env.nttp_bindings`.

## Suggested Next

Start Step 2 by adding the structured NTTP carrier beside existing
`NttpBindings` and wiring one metadata-backed consteval/template call path so
`ConstEvalEnv::nttp_bindings_by_text` or `nttp_bindings_by_key` is populated
from HIR, while leaving the rendered map as named compatibility.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.
- Do not count mangled/link-visible strings or HIR dump strings as cleanup
  targets unless they are used to recover semantic identity.
- Record-layout cleanup is a separate boundary from NTTP cleanup: HIR has
  `HirRecordOwnerKey`/`struct_def_owner_index`, but Sema consteval does not yet
  receive a structured layout map.

## Proof

Delegated proof passed: `git diff --check -- todo.md`.
