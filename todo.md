Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine String Overloads and AST Projection Bridges

# Current Packet

## Just Finished

Plan Step 4 `Quarantine String Overloads and AST Projection Bridges` finished
as a helper quarantine/audit packet. `parser.hpp` and `core.cpp` now explicitly
classify string-only visible typedef/value helpers, string-only visible
resolution helpers, `resolve_visible_*_name`, and
`lookup_*_in_context(..., std::string*)` overloads as compatibility,
projection, or fallback bridges layered over TextId-aware or
`VisibleNameResult` structured paths.

No semantic behavior changed in this packet. The helper contracts now document
that parser-owned token/semantic callers should prefer TextId, namespace
context, `QualifiedNameKey`, or `VisibleNameResult` paths when available.

## Suggested Next

Step 4 is ready for supervisor validation/review. If accepted, advance to the
next plan step or close/split according to the active runbook.

## Watchouts

Remaining raw-string categories are explicit compatibility/projection cases:
tag/ref strings such as `TypeSpec::tag`, enum/tag projection, final
`resolve_visible_*_name` spelling projection, `std::string*` lookup projection
over `VisibleNameResult`, and public string entry points for callers that have
only spelling. Future packets should convert additional call sites only when a
correct token `TextId`, namespace context id, or semantic key can be carried
without changing final AST spelling semantics.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|using_namespace_directive_parse|local_value_shadows_using_alias_assign_expr_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
