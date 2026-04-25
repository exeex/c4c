Status: Active
Source Idea Path: ideas/open/95_parser_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Structured Template Instantiation De-Dup Keys

# Current Packet

## Just Finished

Step 5 - Add Structured Template Instantiation De-Dup Keys follow-up completed.
`TemplateInstantiationKey` now stores a structured vector of type/value argument
entries built from `ParsedTemplateArg` data instead of deriving the argument
identity from the rendered legacy instance key suffix. Both
`ensure_template_struct_instantiated_from_args` and the direct concrete struct
emission guard now use the same canonical argument-key builder.

Focused parser coverage now also proves direct-emission structured-present /
rendered-missing mirror healing: clearing only the rendered legacy set is
detected, the rendered mirror is restored, and the already-instantiated tag is
reused.

## Suggested Next

Next coherent packet: supervisor review, then the next plan step if accepted.
This Step 5 follow-up is ready for review with the focused parser proof passing.

## Watchouts

- Direct emission now treats either mirrored key family as sufficient and heals
  the missing side before returning reuse, matching the helper-path mirror
  behavior.
- The structured argument entry still uses the existing canonical type-key
  spelling and NTTP value/deferred-expression payloads as the parser's current
  structured `ParsedTemplateArg` identity.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
