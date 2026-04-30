# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 fixed the remaining migrated `frontend_parser_tests.cpp` assertions
after the parser semantic lookup API removals. Direct `QualifiedNameKey`
semantic checks now require explicit structured storage, concept and
alias-template fixtures use parser-owned `TextId` state, and value lookup tests
no longer treat rendered compatibility names as semantic authority.

No parser implementation compatibility wrappers or removed fallback semantic
APIs were reintroduced. Type projection bridge assertions remain explicit where
current visible-name APIs intentionally require rendered spelling.

## Suggested Next

Supervisor can review and commit this Step 2 test-migration slice, then choose
the next parser rendered-string compatibility route outside the now-green
frontend parser/Sema subset.

## Watchouts

`lookup_type_in_context`, visible-name projection helpers, and NTTP/default or
template de-dup rendering mirrors still intentionally accept rendered spelling
where their current APIs require it. This packet kept those projection bridges
separate from removed direct semantic lookup routes.

## Proof

Ran delegated proof:
`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_(positive_parser|positive_sema|negative_tests))' --output-on-failure) > test_after.log 2>&1`

Result: passed, 927/927 tests green. Proof log: `test_after.log`.
