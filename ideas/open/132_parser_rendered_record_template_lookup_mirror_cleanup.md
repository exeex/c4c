# Parser Rendered Record Template Lookup Mirror Cleanup

Status: Open
Created: 2026-04-29

Parent Ideas:
- [129_parser_intermediate_carrier_boundary_labeling.md](/workspaces/c4c/ideas/open/129_parser_intermediate_carrier_boundary_labeling.md)
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Remove semantic dependence on rendered record and template lookup mirror strings
inside parser-side record/template definition and instantiation paths.

## Why This Idea Exists

The parser now has structured identity carriers for record and template names,
but Step 3 of idea 129 found compatibility tables that still carry rendered
spelling beside those identities. Those strings are acceptable as transitional
mirrors only when they cannot decide semantic lookup or substitution after a
structured identity exists.

Suspicious paths:

- `ParserDefinitionState::defined_struct_tags`
- `ParserDefinitionState::struct_tag_def_map`
- `ParserTemplateState::template_struct_defs`
- `ParserTemplateState::template_struct_specializations`
- `ParserTemplateState::instantiated_template_struct_keys`
- `ParserTemplateState::nttp_default_expr_tokens`

## Scope

- Trace parser record-definition lookup, constant-evaluation, template
  specialization, and template instantiation paths that still consult rendered
  tag/template strings.
- Make `QualifiedNameKey`, `TextId`, parser symbol ids, direct `StructDef*`
  references, or equivalent structured carriers the semantic authority.
- Keep rendered names only as compatibility mirrors, diagnostics, debug output,
  or generated artifact spelling when they no longer decide lookup.
- Add focused tests for nearby same-feature cases, not only the first observed
  failing or suspicious path.

## Out Of Scope

- Redesigning parser template instantiation.
- Removing generated mangled names that are final artifact spelling.
- Bulk deleting compatibility fields before all consumers have structured
  replacements.
- Changing AST/Sema/HIR ingress behavior except where required by the parser
  producer contract.

## Acceptance Criteria

- Parser record/template semantic lookup no longer depends on rendered tag or
  template spelling when a structured identity is available.
- Compatibility mirrors are documented or named as non-authoritative, or are
  removed after all semantic users are gone.
- Mismatch counters or fallback diagnostics prove any remaining string path is
  compatibility-only.
- Focused parser/frontend tests cover namespace-qualified records, template
  specializations, and NTTP default-expression paths that previously crossed
  this boundary through rendered spelling.
