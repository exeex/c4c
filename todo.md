Status: Active
Source Idea Path: ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Close Audit Evidence

# Current Packet

## Just Finished

Step 5 closed the audit evidence for idea 131 by consolidating the Step 1-4
inventory and checking that every suspicious string-authority family has focused
open follow-up coverage.

Full inventory outcome:

- Parser, Sema, HIR, LIR, BIR, and backend handoff surfaces were classified as
  structured identity, legitimate display/final/generated text, compatibility
  fallback, or suspicious authority.
- Structured carriers already cover the intended cross-IR identity path where
  available: parser `TextId`, `QualifiedNameKey`, parser symbol ids,
  namespace context ids, AST text/qualifier fields, Sema structured lookup
  keys and `*_by_text` mirrors, HIR owner/member/template keys,
  `MemberSymbolId`, `LinkNameId`, LIR `StructNameId`/`LirStructDecl`, BIR
  `BlockLabelId`, and prepared/backend ID-keyed name tables.
- Legitimate text was distinguished from authority: diagnostics, debug dumps,
  route notes, validation errors, parse/debug context, AST/HIR/LIR/BIR
  printers, source literal text, inline asm payloads and constraints,
  operator spelling, mangled/generated specialization names, target triples,
  LLVM/final type spellings, string-pool names and bytes, register names,
  emitted labels, private data labels, and final LLVM/assembly text are
  acceptable when they do not decide semantic lookup or producer/consumer
  identity.
- Compatibility fallback remains acceptable only where a structured-primary
  path exists or where raw/hand-built legacy payloads do not provide structured
  producer data. Recorded examples include parser visible-name and namespace
  spelling mirrors, parser-produced AST/template payload strings, Sema/HIR
  rendered lookup fallbacks with structured mirrors, HIR compile-time registry
  rendered overloads when declaration keys are unavailable, LIR raw
  function/global/extern/callee names when `LinkNameId` is invalid, BIR
  raw-only block labels, textual pointer initializer fallback that resolves to
  `LinkNameId` when available, and prepared helper overloads that immediately
  resolve spelling through `PreparedNameTables`.

Suspicious family to follow-up mapping:

- Step 1 confirmed the existing Parser/Sema/HIR queue covered the original
  suspicious families from idea 131: ideas 132-136 are focused, non-duplicate
  slices, with idea 134 intentionally overlapping Sema/HIR only at the
  parser-produced AST payload boundary.
- Idea 132 covers parser record/template rendered lookup mirrors:
  `defined_struct_tags`, `struct_tag_def_map`, rendered template primary and
  specialization maps, instantiated-template rendered keys, and rendered NTTP
  default-expression token caches.
- Idea 133 covers parser namespace and visible-name compatibility spelling:
  `VisibleNameResult::compatibility_spelling`,
  `UsingValueAlias::compatibility_name`,
  `compatibility_namespace_name_in_context()`, `bridge_name_in_context()`,
  `qualify_name*()`, and string-returning visible/qualified lookup helpers.
- Idea 134 covers parser-produced AST/template payload bridges:
  `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`, `template_arg_nttp_names`,
  `TemplateArgRef::debug_text`, `Node::name`, `Node::unqualified_name`,
  `Node::template_origin_name`, `TypeSpec::tag`,
  `TypeSpec::tpl_struct_origin`, and `TypeSpec::deferred_member_type_name`.
- Idea 135 covers Sema owner/static-member/member and related rendered record
  fallback authority: `resolve_owner_in_namespace_context()`,
  `enclosing_method_owner_struct()`, `lookup_struct_static_member_type()`,
  `has_struct_instance_field()`, record-completeness/key lookup by rendered
  `TypeSpec::tag`, and unqualified variable lookup fallback from `n->name`.
- Idea 136 covers HIR rendered record/member/method/static-member/template
  authority: parsed scoped method names, out-of-class method attachment,
  struct method/member/static-member lookups, template struct primary and
  specialization lookup, pending template-type recovery, and HIR
  static-member/type-trait helpers whose rendered owner lookup remains
  consumer-facing.
- Idea 137 covers the Step 2 parser known-function-name gap:
  `has_known_fn_name(const std::string&)`,
  `register_known_fn_name(const std::string&)`,
  `register_known_fn_name_in_context()` fallback parsing, rendered
  operator/constructor/scoped declaration registration, and rendered
  call-disambiguation probes.
- Idea 138 covers the Step 4 later-IR/backend aggregate layout gap:
  `LirModule::type_decls`, `TypeDeclMap`,
  `compute_aggregate_type_layout()`,
  `lookup_backend_aggregate_type_layout()`,
  `BackendStructuredLayoutTable`, `StructuredTypeSpellingContext`, and
  aggregate/global initializer layout parsing that can still treat final
  `%type` spelling or legacy textual type-decl bodies as layout authority.
- No uncovered suspicious family remains from Steps 1-4 after mapping the
  parser known-function-name family to idea 137 and the later-IR/backend
  aggregate layout family to idea 138.

## Suggested Next

Route to the plan owner for the idea 131 close decision. The executor evidence
indicates the source idea appears ready for plan-owner closure review rather
than another audit runbook, because every suspicious family found by Steps 1-4
is represented by open follow-up ideas 132-138 and the remaining text uses are
classified as structured identity, display/final/generated text, or
compatibility fallback.

## Watchouts

Plan-owner closure should close only idea 131. Follow-up ideas 132-138 must
stay open as implementation cleanup work. Do not reclassify final artifact
spelling, diagnostics, dumps, inline asm, register names, string bytes, or final
assembly as suspicious unless a future implementation pass proves that such
text is deciding semantic lookup or producer/consumer identity.

## Proof

Lifecycle/docs proof only, per packet. No tests were run because no code,
implementation files, or tests changed. Follow-up idea files confirmed present:
ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md,
ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md,
ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md,
ideas/open/135_sema_structured_owner_static_member_lookup_cleanup.md,
ideas/open/136_hir_structured_record_template_lookup_authority_cleanup.md,
ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md,
and
ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md.
`git status --short` after this update shows only `M todo.md`.
