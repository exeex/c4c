# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Demote struct_tag_def_map To Compatibility Mirror

## Just Finished

Step 5 final classification pass found no remaining primary semantic
`struct_tag_def_map` authority after the template static-member blocker fix.

Remaining map use is classified as compatibility/fallback:

- Mirror writers for parsed records, testing hooks, and direct/template
  instantiation spelling keys.
- Template instantiation dedup/reuse checks that preserve rendered spelling
  compatibility while returning `TypeSpec::record_def` when the concrete record
  is known.
- `resolve_record_type_spec()` fallbacks and call sites that now prefer
  `TypeSpec::record_def` before rendered tag lookup.
- Const-eval/layout callers that pass the map so tag-only compatibility types
  still work when no typed record identity is available.
- Legacy nested-owner/member-typedef fallback branches that only run after the
  typed owner path fails.

Added a local `ParserDefinitionState` comment documenting
`struct_tag_def_map` as a rendered-tag compatibility mirror rather than typed
semantic authority.

## Suggested Next

Step 5 is ready for plan-owner review/handoff. Suggested next packet: advance
to Step 6 reproof and return-to-parent-parser-cleanup decision, with supervisor
selecting the broader validation scope.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys as part of this bridge. The retained map still supports tag-only fallback,
testing hooks, rendered instantiation keys, and final spelling compatibility.

Preserve `TypeSpec::tag` as spelling, diagnostics, emitted text, and
compatibility payload; semantic record lookup should continue to flow through
`TypeSpec::record_def` where available.

## Proof

Delegated proof for this packet:

`git diff --check -- todo.md src/frontend/parser/impl/parser_state.hpp src/frontend/parser/impl/support.cpp src/frontend/parser/impl/types/base.cpp src/frontend/parser/impl/types/template.cpp src/frontend/parser/impl/types/declarator.cpp src/frontend/parser/impl/types/struct.cpp src/frontend/parser/impl/declarations.cpp src/frontend/parser/impl/expressions.cpp tests/frontend/frontend_parser_tests.cpp`

Result: passed. This text-only classification packet did not update
`test_after.log`.
