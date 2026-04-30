# Parser AST Template Payload String Bridge Cleanup

Status: Closed
Created: 2026-04-29
Closed: 2026-04-30

Parent Ideas:
- [129_parser_intermediate_carrier_boundary_labeling.md](/workspaces/c4c/ideas/open/129_parser_intermediate_carrier_boundary_labeling.md)
- [130_sema_hir_ast_ingress_boundary_audit.md](/workspaces/c4c/ideas/open/130_sema_hir_ast_ingress_boundary_audit.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Separate parser-produced AST/template display strings from semantic payload
authority at the parser-to-AST and AST-to-Sema/HIR boundary.

## Why This Idea Exists

Step 3 of idea 129 found parser payload fields where rendered names are written
into AST or template helper structures even though structured identity carriers
exist nearby. Some fields are legitimate display or compatibility text, but the
same strings can still feed lookup, substitution, or deferred member-type
decisions downstream.

Suspicious parser/template payload bridges:

- `ParserAliasTemplateInfo::param_names`
- `ParserTemplateArgParseResult::nttp_name`
- `$expr:` text copied into `template_arg_nttp_names`
- `TemplateArgRef::debug_text`

Suspicious AST boundary fields populated by parser code:

- `Node::name`
- `Node::unqualified_name`
- `Node::template_origin_name`
- `TypeSpec::tag`
- `TypeSpec::tpl_struct_origin`
- `TypeSpec::deferred_member_type_name`

## Scope

- Trace parser writes into AST and template payload fields and classify each
  field as semantic authority, display text, compatibility fallback, or
  generated spelling.
- Prefer `TextId`, `QualifiedNameKey`, `TemplateParamId`, direct symbol/record
  references, or existing structured template argument fields for semantic
  lookup and substitution.
- Coordinate with Sema/HIR ingress auditing from idea 130 where parser-produced
  AST spelling is consumed downstream.
- Add tests that cover deferred member types, alias templates, NTTP names, and
  template-origin payloads without relying on rendered spelling as the only
  identity.

## Out Of Scope

- Removing AST display fields before downstream consumers are migrated.
- Replacing every diagnostic or dump string.
- Reworking final emitted symbol names or generated mangling.

## Acceptance Criteria

- Parser-produced AST/template payload strings are either documented as
  display/compatibility-only or replaced by structured semantic carriers at
  lookup and substitution sites.
- Downstream Sema/HIR consumers no longer need to reconstruct semantic identity
  from rendered AST spelling where parser structured identity is available.
- Focused tests prove the cleanup across alias-template parameters, NTTP
  payloads, deferred member types, and template-origin names.

## Closure Notes

Closed after active plan Step 5 validation. Focused acceptance rebuilt the
default preset and passed 1066/1066 delegated parser, Sema, HIR, C++ parser,
negative, and selected C torture tests. Remaining string-payload paths are
documented as fallback-only compatibility or display/debug carriers, not silent
semantic authority.
