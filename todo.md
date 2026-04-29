Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine String Overloads and AST Projection Bridges

# Current Packet

## Just Finished

Plan Step 4 `Quarantine String Overloads and AST Projection Bridges` continued.
`parse_base_type` now carries the current identifier token `TextId` into the
visible value lookup used by the expression/type probe path, replacing
`find_visible_var_type(id)` with `find_visible_var_type(id_text_id, id)`.

Earlier Step 4 work already removed one-argument `resolve_visible_type` calls
from the owned type/template files. This slice adds another token-backed
semantic visible lookup conversion without changing final AST spelling.

## Suggested Next

Continue Step 4 by auditing remaining string-only visible typedef lookups that
come from tag/ref strings. Convert only call sites where the originating token
`TextId` or parser-owned semantic key can be carried without confusing final
AST projection spelling.

## Watchouts

Remaining string-only calls observed in `types/base.cpp` are classified as:
member typedef recursion over `TypeSpec::tag`, enum/tag projection,
`ts.tag` combined-specifier resolution, and serialized template arg refs. They
do not have a clearly correct token `TextId` in this bounded slice.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|using_namespace_directive_parse|local_value_shadows_using_alias_assign_expr_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
