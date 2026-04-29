# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split Parser Semantic Lookup From Text Spelling

## Just Finished

Completed Step 3's first parser record-tag semantic packet as an inventory and
blocker slice for `DefinitionState::struct_tag_def_map`.

Inventory and classification of each owned read/write path:
- `parser_state.hpp`: `struct_tag_def_map` is the rendered-tag-to-`Node*`
  bridge. The key is spelling/final rendered tag text; the value is the parser
  record identity currently available to lookup users.
- `support.cpp`: `Parser::eval_const_int_with_parser_tables`,
  `field_align`, `struct_align`, `struct_sizeof`, and `compute_offsetof` use
  `TypeSpec::tag` strings to reach the record `Node*` for layout and
  `offsetof`. These are semantic record lookups, but their public/helper
  contract only receives `TypeSpec` plus a string-keyed map.
- `support.cpp`: `register_struct_definition_for_testing` is a test
  compatibility writer for rendered tags.
- `declarations.cpp`: both local/global incomplete-object checks read
  `TypeSpec::tag` through the map to decide whether a record definition has a
  body; global constexpr evaluation passes the same bridge into const eval.
- `expressions.cpp`: `offsetof` constant folding passes the map into
  `eval_const_int`; the `NK_OFFSETOF` node carries the record only as
  `TypeSpec::tag`.
- `types/struct.cpp`: record definition registration writes both source
  spelling and canonical `sd->name` spellings to the map; bit-field width
  evaluation only passes the bridge through const eval.
- `types/template.cpp`: template instantiation writes mangled rendered tags
  for explicit specializations and injected parses; static member lookup reads
  instantiated/base records by rendered tag and then recurses on `Node*`.
- `types/declarator.cpp`: qualified/nested owner resolution reads the map from
  resolved owner spelling or nested `TypeSpec::tag`; array-size evaluation only
  passes the bridge through const eval.
- `types/base.cpp`: member typedef lookup, template base instantiation,
  direct template emission, and `StructLike::type` remapping read/write mangled
  rendered tags while preserving `TypeSpec::tag` as the outward bridge.
- `frontend_parser_tests.cpp`: direct-emission tests intentionally mutate and
  assert `struct_tag_def_map` as the compatibility/final-spelling mirror for
  template instantiation recovery.

No parser-owned conversion was made. The blocker is that the first viable
semantic-record surfaces all cross through `TypeSpec`, and `TypeSpec` currently
carries record identity only as `const char* tag`. It has no parser-owned
record pointer, record id, or typed semantic key in the delegated owned files.
Changing the map behind these call sites would still make raw rendered tag text
the authority, while adding real semantic identity requires changing the shared
AST/type contract or downstream consumers that depend on `TypeSpec::tag`.

## Suggested Next

Plan-owner or supervisor follow-up: split a separate bridge initiative for
typed parser record identity through `TypeSpec` and downstream type consumers.
The smallest useful next design packet should define how a parsed record
definition `Node*` or stable parser record id travels with struct/union
`TypeSpec` values while preserving `TypeSpec::tag` as final spelling and
compatibility output.

## Watchouts

Do not convert `struct_tag_def_map` to a differently named string table or a
`TextId` table as Step 3 progress. That would still treat spelling as semantic
record authority because the lookup input is `TypeSpec::tag`.

Do not remove or weaken the existing template direct-emission tests that mutate
`struct_tag_def_map`; they document the currently required compatibility mirror
until typed record identity exists.

Preserve `TypeSpec::tag` strings as explicit final-spelling/downstream bridges
for sizeof/alignof/offsetof, template instantiations, member typedef lookup, and
HIR-facing type surfaces.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' > test_after.log 2>&1`

Proof log: `test_after.log`
