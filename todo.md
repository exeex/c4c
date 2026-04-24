Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 3 accessor slice for parser diagnostic status.
- Added `Parser::had_parse_error() const` as a narrow public parser diagnostic
  accessor that exposes only parse-error status, not `ParserDiagnosticState`.
- Switched `src/apps/c4cll.cpp` from reading
  `parser.diagnostic_state_.had_error` directly to calling
  `parser.had_parse_error()`.
- Confirmed the remaining `diagnostic_state_.had_error` reads/writes in this
  slice are parser-internal.
- Changed files: `todo.md`, `src/frontend/parser/parser.hpp`,
  `src/frontend/parser/parser_core.cpp`, and `src/apps/c4cll.cpp`.

## Suggested Next

- Continue Step 3 by auditing non-production/test parser state reads and decide
  the next narrow facade accessor needed before moving `ParserDiagnosticState`
  behind a private state handle.

## Watchouts

- Remaining facade/PIMPL blocker: `parser.hpp` still includes
  `impl/parser_state.hpp` because the public `Parser` object owns state fields
  directly, and app/test code still reaches some of those fields. This packet
  only removed the known production `c4cll` read of diagnostic state; it did not
  change parser state layout or semantics.
- `Parser::had_parse_error()` intentionally exposes a boolean status only. Avoid
  widening it to return `ParserDiagnosticState` or a mutable diagnostic handle.

## Proof

- Ran exactly:
  `{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`
- Result: passed.
- Test subset used: `frontend_parser_tests`.
- Proof log path: `test_after.log`.
