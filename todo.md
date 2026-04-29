Status: Active
Source Idea Path: ideas/open/129_parser_intermediate_carrier_boundary_labeling.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate behavior preservation

# Current Packet

## Just Finished

Step 4: Queued durable follow-up coverage for the suspicious parser-side
cross-stage string authority found in Step 3.

Follow-up mapping:
- `ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md`
  owns rendered record/template lookup mirrors:
  `ParserDefinitionState::{defined_struct_tags, struct_tag_def_map}` and
  `ParserTemplateState::{template_struct_defs, template_struct_specializations,
  instantiated_template_struct_keys, nttp_default_expr_tokens}`.
- `ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md`
  owns namespace and visible-name compatibility spelling:
  `Parser::VisibleNameResult::compatibility_spelling`,
  `ParserNamespaceState::UsingValueAlias::compatibility_name`,
  `compatibility_namespace_name_in_context`, and string overloads of namespace
  and visible-name lookup helpers.
- `ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md`
  owns parser-produced AST/template payload bridges:
  `ParserAliasTemplateInfo::param_names`,
  `ParserTemplateArgParseResult::nttp_name`, `$expr:` text copied into
  `template_arg_nttp_names`, `TemplateArgRef::debug_text`, and parser writes to
  `Node::name`, `Node::unqualified_name`, `Node::template_origin_name`,
  `TypeSpec::tag`, `TypeSpec::tpl_struct_origin`, and
  `TypeSpec::deferred_member_type_name`.

Existing ideas `130_sema_hir_ast_ingress_boundary_audit.md` and
`131_cross_ir_string_authority_audit_and_followup_queue.md` remain audit/queue
coverage. They are not concrete cleanup ownership for these parser producer
paths, so Step 4 created separate narrow follow-up ideas.

## Suggested Next

Delegate Step 5 to validate behavior preservation for this documentation and
idea-queueing slice. No implementation files were changed by Step 4.

## Watchouts

- Step 4 intentionally changed only `todo.md` and new files under
  `ideas/open/`.
- The new ideas are cleanup follow-ups, not proof that every listed path is a
  bug. Each idea asks future execution to separate compatibility/display text
  from semantic authority.
- `plan.md` was left unchanged because the Step 4 runbook contract was already
  consistent.

## Proof

No build or test proof was required for this lifecycle-only idea queueing
packet. No tests were run and no new `test_after.log` was produced.
