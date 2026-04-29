# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Reprove And Return To Parser Cleanup

## Just Finished

Plan-owner review accepted Step 5 as complete. The latest Step 5
classification records no remaining primary semantic `struct_tag_def_map`
authority after the template static-member blocker fix.

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

Step 6 supervisor packet: reprove the typed parser record identity bridge and
decide the return path to parent parser cleanup idea 123.

Concrete expected work:

- Run the focused typed-record and compatibility fallback parser/frontend proof
  subset against the current bridge state, producing or refreshing
  `test_after.log` as appropriate for the supervisor-selected regression guard.
- Run broader parser-adjacent frontend validation appropriate for the
  `TypeSpec` contract and template/static-member lookup changes.
- Review the bridge diff for expectation downgrades or testcase-shaped
  shortcuts; reject any overfit before lifecycle closure or parent-plan return.
- If proof is clean, ask the plan owner to close or park idea 127 and resume or
  regenerate parent parser cleanup idea 123 with `struct_tag_def_map`
  classified as compatibility/final-spelling mirror.

## Watchouts

Do not delete `struct_tag_def_map` or remove rendered template compatibility
keys as part of this bridge. The retained map still supports tag-only fallback,
testing hooks, rendered instantiation keys, and final spelling compatibility.

Preserve `TypeSpec::tag` as spelling, diagnostics, emitted text, and
compatibility payload; semantic record lookup should continue to flow through
`TypeSpec::record_def` where available.

Step 6 should be validation and lifecycle routing, not another semantic
conversion packet, unless reproof exposes a real covered-path gap.

## Proof

Delegated proof for this packet:

`git diff --check -- todo.md src/frontend/parser/impl/parser_state.hpp src/frontend/parser/impl/support.cpp src/frontend/parser/impl/types/base.cpp src/frontend/parser/impl/types/template.cpp src/frontend/parser/impl/types/declarator.cpp src/frontend/parser/impl/types/struct.cpp src/frontend/parser/impl/declarations.cpp src/frontend/parser/impl/expressions.cpp tests/frontend/frontend_parser_tests.cpp`

Result: passed. This text-only classification packet did not update
`test_after.log`.

Plan-owner decision: Step 5 completion accepted; `plan.md` was already aligned
with Step 6 and did not need a rewrite.
