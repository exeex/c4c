# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 removed the parser rendered-string conflict route
`Parser::has_conflicting_user_typedef_binding(const std::string&, const TypeSpec&)`.
The conflict predicate and its user-typedef predicate now take parser-owned
`TextId` values, typedef declaration call sites pass the `parse_declarator`
text IDs directly, and the parser fixture asserts through the same `TextId`
authority.

## Suggested Next

Supervisor can review and commit this Step 2 parser API slice, then choose the
next parser rendered-string compatibility route still outside this conflict
lookup path.

## Watchouts

`find_visible_typedef_type(TextId)` remains the conflict route's lookup
authority, so local visible typedefs and structured/visible type resolution stay
covered without reintroducing string-name semantic probing. Other projection
bridges that intentionally accept rendered spelling were left untouched.

## Proof

Ran delegated proof:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
