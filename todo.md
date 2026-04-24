Status: Active
Source Idea Path: ideas/open/92_parser_agent_index_header_hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Minimize Public Exposure Of Parser State

# Current Packet

## Just Finished

- Completed plan Step 3 inventory-only audit of remaining external direct
  `Parser` state access outside parser implementation `.cpp` internals.
- Production app consumers: no direct state field access remains under
  `src/apps`; `c4cll` constructs `Parser`, sets debug channels, and uses the
  public `had_parse_error()` facade.
- Frontend/HIR consumers: no direct state field access remains under
  `src/frontend/hir`; the only non-`.cpp` frontend state reads/writes are in
  private parser helper header `src/frontend/parser/impl/types/types_helpers.hpp`.
- Parser-private helper access in `impl/types/types_helpers.hpp`: `tokens_`
  and `pos_` are used for injected-token fixture-style reparsing and lookahead;
  `definition_state_` is read for defined-struct-tag lookup. This is private
  implementation access, not public API, but it still depends on `parser.hpp`
  exposing those fields because the helper is a header.
- Parser tests still directly use these public fields:
  `shared_lookup_state_`, `active_context_state_`, `tokens_`, `pos_`,
  `binding_state_`, `definition_state_`, `namespace_state_`, and
  `template_state_`.
- Classification:
  `tokens_`/`pos_` test access is mostly fixture construction and cursor
  assertions around helper-built token streams; accidental implementation
  access unless wrapped by a test/debug token-stream facade.
  `shared_lookup_state_` access is white-box verification of symbol/name table
  internals; true test/debug API, not production API.
  `active_context_state_` access is white-box verification of TextId-preferred
  bookkeeping and stale-string fallback behavior; true test/debug API.
  `binding_state_`, `definition_state_`, `namespace_state_`, and
  `template_state_` writes are fixture shortcuts that seed parser internals;
  accidental implementation access and good facade candidates.

## Suggested Next

- Code-changing Step 3 sub-slice: add a narrow parser test/support facade for
  injected token streams and cursor observation, then convert the
  `frontend_parser_tests.cpp` cases that directly assign `parser.tokens_` and
  `parser.pos_` to that facade. Keep behavior and expectations unchanged; do
  not touch parser semantics or testcase contracts.

## Watchouts

- The `impl/types/types_helpers.hpp` direct `tokens_`/`pos_` usage is
  implementation-internal despite living in a header; do not classify it as a
  public caller. It is still a facade/PIMPL blocker because a future private
  state split must give parser helper headers a private cursor interface.
- Do not start by exposing whole state carriers. The useful next slice is a
  narrow token-stream/cursor helper because it removes the highest-volume test
  access family without changing expectations.
- No remaining `diagnostic_state_`, `core_input_state_`, `arena_`,
  `lexical_scope_state_`, or `pragma_state_` direct external access was found
  in the audited paths.

## Proof

- Inventory-only packet; no build proof required and no proof log created or
  modified.
- Ran:
  `rg -n "\bparser\.(core_input_state_|tokens_|pos_|arena_|shared_lookup_state_|binding_state_|definition_state_|template_state_|lexical_scope_state_|active_context_state_|namespace_state_|diagnostic_state_|pragma_state_)" src/apps`
  Result: no matches.
- Ran:
  `rg -n "\bparser\.(core_input_state_|tokens_|pos_|arena_|shared_lookup_state_|binding_state_|definition_state_|template_state_|lexical_scope_state_|active_context_state_|namespace_state_|diagnostic_state_|pragma_state_)" src/frontend -g '!**/parser_*.cpp'`
  Result: matches only in
  `src/frontend/parser/impl/types/types_helpers.hpp` for `tokens_`, `pos_`,
  and `definition_state_`.
- Ran:
  `rg -n "\bparser\.(core_input_state_|tokens_|pos_|arena_|shared_lookup_state_|binding_state_|definition_state_|template_state_|lexical_scope_state_|active_context_state_|namespace_state_|diagnostic_state_|pragma_state_)" tests/frontend`
  Result: matches only in `tests/frontend/frontend_parser_tests.cpp`.
- Ran:
  `rg -o "parser\.(core_input_state_|tokens_|pos_|arena_|shared_lookup_state_|binding_state_|definition_state_|template_state_|lexical_scope_state_|active_context_state_|namespace_state_|diagnostic_state_|pragma_state_)" tests/frontend/frontend_parser_tests.cpp | sort | uniq -c`
  Result: `tokens_` 32, `pos_` 23, `shared_lookup_state_` 16,
  `namespace_state_` 12, `active_context_state_` 6, `template_state_` 3,
  `binding_state_` 1, `definition_state_` 1.
