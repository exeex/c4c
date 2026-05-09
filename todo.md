Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Template Owner And Substitution String Authority

# Current Packet

## Just Finished

Completed Step 1 inventory for template owner and substitution string
authority. Direct `ParserTemplateScopeFrame::owner_struct_tag` uses are:
definition in `src/frontend/parser/parser_types.hpp`, assignment in
`relabel_free_function_template_as_owner_scope`, and a display assertion in
`frontend_parser_lookup_authority_tests`; the assignment is already gated by
structured `QualifiedNameRef` lookup and the stored string is a display
projection, not the semantic owner key.

Classifications:

- `owner_struct_tag`: display/debug projection. Retain for parsed owner
  spelling and compatibility assertions; removal is not required for this
  plan, but Step 2 should add a structured owner key beside it so no future
  caller needs to recover owner identity from the string.
- `src/frontend/parser/impl/types/base.cpp` NTTP array-size loops at the
  `member_ts.array_size_expr->name == pname/npname` and `ase->name == npname`
  sites: semantic lookup today. These are the primary repair targets for the
  NTTP packet because rendered names can bind across owner/parameter domains.
- Member typedef instantiation in `base.cpp` cloning/registering concrete
  member typedefs: mixed semantic lookup plus compatibility bridge. The
  concrete `register_template_instantiation_member_typedef_binding` path is
  semantic and should remain keyed by concrete owner plus member `TextId`; the
  legacy tag/string substitutions around it are compatibility fallbacks and
  should not override valid `tag_text_id`, `template_param_text_id`,
  owner-key, or member-text metadata.
- Direct record member typedef lookup over `member_typedef_names`: semantic
  lookup when a member `TextId` is present; compatibility bridge only when no
  member `TextId` exists. Retain the no-metadata fallback until all parser
  writers populate `member_typedef_text_ids`.
- `deferred_member_type_name`, legacy TypeSpec tags, and rendered
  `tpl_struct_origin` strings: syntax payload/final spelling/debug projection,
  with compatibility use only when structured owner/member metadata is absent.
  They must not recover after a complete structured owner/member miss.

Nearby coverage already exists for stale owner metadata, same-spelling
member typedefs, alias-template member typedef substitution, NTTP TextId
carriers, and rendered NTTP fallback rejection in
`frontend_parser_lookup_authority_tests`, `frontend_parser_tests`,
`frontend_hir_lookup_tests`, and the focused
`cpp_hir_parser_type_base_*_metadata_test` targets.

## Suggested Next

First implementation packet target: Step 2 structured owner carrier. Add a
`QualifiedNameKey`/owner metadata field beside
`ParserTemplateScopeFrame::owner_struct_tag`, populate it in
`relabel_free_function_template_as_owner_scope`, and route any owner-identity
consumer through the structured key while leaving `owner_struct_tag` as parsed
display spelling.

Suggested focused proof command for that packet:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$' --output-on-failure`

## Watchouts

- Do not treat rendered template owner/member/NTTP names as semantic authority
  when owner, parameter-domain, or member `TextId` metadata exists.
- Do not weaken tests or add named-fixture shortcuts to make a narrow case
  pass.
- Keep `owner_struct_tag`, syntax payloads, and display strings only as
  bounded compatibility/debug/final-spelling paths unless no structured
  carrier exists.
- The later NTTP repair packet should demote the `base.cpp` array-size
  rendered-name loops behind parameter-domain metadata: owner key or owner
  TextId, parameter index, `TemplateParamDomainKind::NonType`, and parameter
  `TextId`. Retain string matching only as a legacy no-metadata bridge.
- Member typedef repairs should preserve the existing concrete owner/member
  key registration and only remove rendered owner/member recovery that can
  override complete structured metadata.

## Proof

Inventory-only packet; no build required and no `test_after.log` was created
because the supervisor explicitly marked test logs as do-not-touch. Read-only
proof consisted of focused `rg` and `sed` inspection of `owner_struct_tag`,
NTTP array-size substitution loops, member typedef lookup/instantiation paths,
and nearby frontend/HIR coverage.
