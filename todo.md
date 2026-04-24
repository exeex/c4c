Status: Active
Source Idea Path: ideas/open/86_parser_alias_template_structured_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Audit And Broader Parser Proof

# Current Packet

## Just Finished

Step 5 final audit completed for the active-context structured identity
runbook. Re-ran the active-context field search for `last_using_alias_name`,
`last_using_alias_key`, `last_resolved_typedef`, `current_struct_tag`, and the
template-struct bridge helpers.

Remaining active-context string mirrors are classified as bounded support:

- `last_using_alias_name`: no live parser source call site consumes
  `last_using_alias_name_text()`. The only setter path is structured
  `set_last_using_alias_name(const QualifiedNameKey&)`; the retained string
  field is fallback storage for the compatibility accessor.
- `last_using_alias_key`: the live alias-template handoff in
  `parser_declarations.cpp` uses the structured key after `parse_top_level()`
  registers a using-alias.
- `last_resolved_typedef`: remaining string reads are fallback/snapshot support
  for tentative restore and alias-template bridge spelling. Semantic follow-up
  paths use `last_resolved_typedef_text_id` for function-pointer typedef info
  and structured alias-template lookup before legacy rendered-name recovery.
- `current_struct_tag`: remaining direct string snapshots preserve rollback
  fallback spelling. Remaining accessor uses are member-owner registration,
  constructor/destructor injected/source-name comparison, incomplete self-type
  checks, and template/member parsing presence gates via
  `current_struct_tag_text()`.
- Template-struct identity work was not reopened: primary and specialization
  lookups still route through structured key helpers first, and the remaining
  `instantiated_template_struct_keys` usage is documented as an emitted
  concrete-struct duplicate guard after primary/specialization selection.

## Suggested Next

Supervisor lifecycle decision for the exhausted active-context runbook. Ask
whether broader validation is needed before close or deactivation.

## Watchouts

- No implementation inconsistency was found within the delegated audit scope.
- Remaining rendered alias/typedef names are compatibility bridge or fallback
  spellings, not primary semantic identity.
- Broader validation policy belongs to the supervisor; this executor packet ran
  only the delegated parser subset proof.

## Proof

Delegated proof passed:

`bash -lc 'set -o pipefail; cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '\''^frontend_parser_tests$'\'' | tee test_after.log'`

Proof log: `test_after.log`.
