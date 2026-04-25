Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Structured Template Instantiation De-Dup Keys

# Current Packet

## Just Finished

Step 5 - Add Structured Template Instantiation De-Dup Keys completed. Added
`ParserTemplateState::TemplateInstantiationKey` keyed by `QualifiedNameKey`
plus canonical argument signature beside the rendered-string instantiation
de-dup set, and mirrored both parser-owned instantiation de-dup paths:
`ensure_template_struct_instantiated_from_args` and the direct concrete struct
emission guard in `types/base.cpp`.

Focused parser coverage now proves explicit specialization/helper reuse and
direct primary-template emission both populate the structured and rendered key
families, keep the rendered legacy set behavior-compatible, and detect/heal a
missing structured mirror.

## Suggested Next

Next coherent packet: supervisor review or the next plan step. This slice is
ready for review with the focused parser proof passing.

## Watchouts

- The direct emission guard remains legacy-led for behavior compatibility:
  rendered-present skips emission and heals a missing structured mirror;
  rendered-missing still emits and then marks both key families.
- `rg 'instantiated_template_struct_keys' src/frontend/parser/impl` now shows
  structured mirror checks/inserts beside both parser-owned rendered-set paths.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
