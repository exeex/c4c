# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 removed the parser string lookup overload family
`find_identifier(std::string_view)` from `ParserSymbolTable` and
`ParserNameTables`. The remaining parser name-table lookup caller now uses its
parser-owned `TextId`, and the parser fixture assertions check symbol lookup
through `TextId` values instead of rendered strings.

## Suggested Next

Supervisor can review and commit this Step 2 parser API slice, then choose the
next parser rendered-string compatibility route still outside the removed
parser name-table lookup overload path.

## Watchouts

`ParserSymbolTable::intern_identifier(std::string_view)` and
`ParserNameTables::intern_identifier(std::string_view)` remain because this
packet only owned removal of semantic lookup overloads. Other projection bridges
that intentionally accept rendered spelling were left untouched.

## Proof

Ran delegated proof:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
