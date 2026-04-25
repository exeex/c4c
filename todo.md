Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit record-owner string identity surfaces

# Current Packet

## Just Finished

Completed Step 1 audit of HIR record-owner string identity surfaces.

Inventory and classification:
- `TypeSpec::tag` (`src/frontend/parser/ast.hpp`) is the rendered tag/typedef spelling carried through HIR and codegen. Classification: bridge-required when used as semantic owner identity; codegen spelling when passed to LLVM type rendering; diagnostic spelling when appended to messages.
- `Module::struct_defs` and `Module::struct_def_order` (`src/frontend/hir/hir_ir.hpp`) are keyed/ordered by rendered `SymbolName`. Writers: ordinary `lower_struct_def` and template `instantiate_template_struct_body`. Readers span HIR semantic lookup, HIR dumps, layout, ABI classification, and HIR-to-LIR type declaration emission. Classification: semantic lookup for HIR reads; codegen spelling for type decl order/name emission; legacy parity proof once structured mirrors exist.
- `HirStructDef::{tag,tag_text_id,ns_qual,base_tags,base_tag_text_ids}` already preserves rendered tag text plus namespace metadata on the definition. Classification: `tag`/`base_tags` are rendered spelling plus current bridge; `tag_text_id`/`ns_qual`/`base_tag_text_ids` are available structured-key material.
- Lowerer AST registries `struct_def_nodes_`, `template_struct_defs_`, and `template_struct_specializations_` are string-keyed by rendered tag or primary template name. Classification: semantic lookup and bridge-required. Metadata availability: AST `Node` carries `is_global_qualified`, qualifier segments/text ids, name, template origin, and template args; `CompileTimeState` already mirrors template struct primary/specialization lookup by declaration identity.
- Lowerer method maps `struct_methods_`, `struct_method_link_name_ids_`, and `struct_method_ret_types_` are keyed as `"tag::method[_const]"`. Writers: `lower_struct_def` and `register_instantiated_template_struct_methods`. Readers: `find_struct_method_mangled`, `find_struct_method_link_name_id`, `find_struct_method_return_type`, direct operator/range/new-delete paths, and out-of-class method attachment. Classification: semantic lookup for resolution/return type; codegen spelling for mangled/link names; legacy parity proof once mirrored.
- Static/member owner maps `struct_static_member_decls_`, `struct_static_member_const_values_`, `find_struct_member_symbol_id`, and `MemberExpr::resolved_owner_tag` are rendered-owner paths. Classification: semantic lookup for static/member resolution and inherited lookup; bridge-required for `resolved_owner_tag` crossing into LIR; diagnostic/HIR dump spelling for printed owner annotations.
- Base-class owner lookup uses rendered `HirStructDef::base_tags` and recursive `module_->struct_defs.find(base_tag)` in HIR and LIR. Classification: semantic lookup plus codegen layout bridge; `base_tag_text_ids` are available metadata but do not yet identify the owner without the owning definition context.
- HIR-to-LIR/codegen consumers (`llvm_ty`, `sizeof_ts`, calling convention, `object_align_bytes`, `build_type_decls`, field-chain lookup, const-init emission) use `TypeSpec::tag` or `struct_def_order` to find rendered struct definitions and emit LLVM names. Classification: mostly codegen spelling/layout lookup; should remain rendered-name fallback during HIR semantic migration.
- HIR inspection uses `struct_def_order` and `resolved_owner_tag` for dumps. Classification: diagnostic spelling; keep rendered.

Metadata availability notes:
- Ordinary HIR struct definitions already have enough source metadata for a structured record-owner key: `HirStructDef::ns_qual`, `tag_text_id`, rendered `tag`, and AST `Node` qualifier/global fields.
- Template struct primary/specialization registration already has a structured declaration-identity bridge in `CompileTimeState`; lowerer-local maps still need dual-write mirrors to avoid relying only on rendered names.
- Instantiated template structs additionally have `TemplateStructInstanceKey`/specialization key material, so their durable structured owner should include primary identity plus concrete template argument identity, with rendered mangled tag retained as output spelling.
- Downstream codegen generally does not have AST owner metadata at call sites; it should consume HIR-provided mirrors/fallbacks rather than reconstructing structured keys from `TypeSpec::tag`.

Recommended first structured mirror target: add the first mirror beside `Module::struct_defs`/`struct_def_order` and lowerer `struct_def_nodes_`, because ordinary `lower_struct_def` already has `HirStructDef::ns_qual` plus `tag_text_id`, and most later method/member/static/base lookups depend on stable struct-definition ownership.

## Suggested Next

Proceed to Step 2 by defining the HIR record-owner key shape and helper construction from `HirStructDef`/AST metadata, with a bridge constructor for template instantiation identity.

## Watchouts

Keep `TypeSpec::tag`, `HirStructDef::tag`, and LLVM type-name rendering as output spelling. Do not route codegen through structured keys directly; first dual-write HIR semantic mirrors and retain rendered-name fallback/mismatch proof. The existing `CompileTimeState` structured template struct registry is useful precedent, but it is declaration-identity based and does not by itself cover `Module::struct_defs`, method maps, static member maps, or LIR field access.

## Proof

Audit-only lifecycle/docs update. Inspected HIR declarations, lowerer map declarations/writers/readers, template struct registration/instantiation paths, member owner handoff, HIR dump output, and HIR-to-LIR/codegen consumers using `rg` plus `c4c-clang-tool-ccdb` function-signature inventory for representative HIR/codegen translation units. No implementation files changed; no build or CTest required, and no `test_after.log` was produced for this docs-only proof.
