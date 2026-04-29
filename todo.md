Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Compatibility Spelling Authority

# Current Packet

## Just Finished

Plan Step 1 `Classify Compatibility Spelling Authority` completed as a
classification-only packet.

Authority classification map:
- `VisibleNameResult::compatibility_spelling`: display/projection bridge after
  local visible value, type, and concept lookup; written by visible result
  construction, using-alias resolution, namespace/type/value/concept lookup
  helpers, qualified fallback results, and `visible_name_spelling` projection.
  It must not override available `QualifiedNameKey`, `TextId`, parser symbol
  ids, or namespace context ids.
- `UsingValueAlias::target_key`: semantic authority for using-value alias
  lookup. `lookup_using_value_alias` already tries structured target rendering
  first through `render_value_binding_name`, `has_known_fn_name`, and
  `find_structured_var_type`.
- `UsingValueAlias::compatibility_name`: fallback-only authority for explicit
  no-key compatibility aliases. It is written by testing helpers and
  `declarations.cpp` using-import handling, and it should not validate a
  missing or mismatched structured `target_key`.
- `lookup_value_in_context`, `lookup_type_in_context`, and
  `lookup_concept_in_context`: structured-primary bridges where `TextId` or
  structured keys exist, with TextId-less branches classified as fallback-only.
- `compatibility_namespace_name_in_context` and `bridge_name_in_context`:
  namespace rendering/projection bridges that still need Step 3 treatment after
  using-alias behavior is tightened.

## Suggested Next

Start `plan.md` Step 2 with the using-value alias lookup/string overload
cleanup. Keep `UsingValueAlias::target_key` as the semantic authority in
`lookup_using_value_alias`, preserve explicit no-key `compatibility_name` as a
named fallback path, and adjust focused parser tests only if needed to make the
structured-target mismatch and no-key fallback contract visible.

## Watchouts

Existing tests already cover corrupted compatibility spelling not overriding
structured using-value alias targets, explicit no-key compatibility fallback,
local shadowing, and namespace import interactions. Avoid expectation rewrites
or testcase-shaped shortcuts; add tests only for a contract gap.

## Proof

Classification-only todo update; no build or test proof required. No
`test_after.log` update was needed for this packet.
