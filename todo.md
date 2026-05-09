Status: Active
Source Idea Path: ideas/open/155_template_scope_owner_and_member_substitution_domain_keys.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Owner Carrier Beside Template Scope Display Text

# Current Packet

## Just Finished

Completed Step 2 structured owner carrier work. Added
`ParserTemplateScopeFrame::owner_struct_key` beside the existing
`owner_struct_tag` display string, populated it in
`relabel_free_function_template_as_owner_scope` from the resolved template
primary's structured `QualifiedNameKey`, and extended the lookup-authority
coverage to assert the owner key remains the semantic carrier while
`owner_struct_tag` stays the parsed display spelling.

## Suggested Next

Next implementation packet target: repair the NTTP array-size substitution
loops identified in Step 1 so parameter substitution prefers structured
owner/parameter-domain metadata before any rendered-name compatibility bridge.

## Watchouts

- `owner_struct_key` is now available only where a free-function template scope
  was relabeled as an enclosing-class template scope; existing display-only
  code may still read `owner_struct_tag`, but no direct semantic consumers of
  the display string remain in the parser files touched by this packet.
- Do not weaken tests or add named-fixture shortcuts to make a narrow case pass.
- The later NTTP repair packet should demote the `base.cpp` array-size
  rendered-name loops behind parameter-domain metadata: owner key or owner
  TextId, parameter index, `TemplateParamDomainKind::NonType`, and parameter
  `TextId`. Retain string matching only as a legacy no-metadata bridge.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -R '^(frontend_parser_lookup_authority_tests|frontend_parser_tests)$' --output-on-failure`

Full build and focused ctest output captured in `test_after.log`.
