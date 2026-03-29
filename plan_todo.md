# Plan Todo

Status: Active
Source Idea: ideas/open/01_cpp17_keyword_support_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 2: evaluate `not_eq` as the next narrow alternative-operator
  spelling slice after `not`, keeping coverage scoped to lexer tokenization
  plus the smallest equality-operator compatibility edge it exposes

## Todo

- [x] Inspect `src/frontend/lexer/token.hpp` and `src/frontend/lexer/token.cpp`
  to list missing C++17 keywords and alternative operator spellings
- [x] Identify parser sites in declarations / expressions / statements / types
  that currently depend on identifier spelling for those words
- [x] Choose the first minimal implementation slice and record the exact target
  test(s)
- [x] Record the current full-suite baseline before implementation work
- [x] Add the narrowest validating lexer or parser test for the first slice
- [x] Implement the first keyword-classification slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Record a fresh full-suite baseline in `test_before.log` before the `and`
  slice
- [x] Add the narrowest validating lexer / parser coverage for the `and` slice
- [x] Implement the `and` alternative-operator slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Record a fresh full-suite baseline before the `mutable` slice
- [x] Add the narrowest validating lexer or parser test for the `mutable` slice
- [x] Implement the `mutable` keyword-classification slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Record a fresh full-suite baseline before the `friend` slice
- [x] Add the narrowest validating lexer or parser test for the `friend` slice
- [x] Implement the `friend` keyword-classification slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Add the narrowest validating lexer or parser test for the next slice
- [x] Implement the next keyword-classification slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Add the narrowest validating lexer or parser test for the following slice
- [x] Implement the following keyword-classification slice
- [x] Re-run targeted tests, nearby coverage, and the full suite
- [x] Record a fresh full-suite baseline in `test_before.log` before the `or`
  slice
- [x] Add the narrowest validating lexer / parser coverage for the `or` slice
- [x] Implement the `or` alternative-operator slice
- [x] Re-run targeted tests, nearby coverage, and the full suite

## Completed

- [x] Activated [plan.md](/workspaces/c4c/plan.md) from
  [ideas/open/01_cpp17_keyword_support_plan.md](/workspaces/c4c/ideas/open/01_cpp17_keyword_support_plan.md)
- [x] Inventory result: the lexer already reserves `template`, `constexpr`,
  `consteval`, `namespace`, `using`, `decltype`, `class`, `true`, `false`,
  `operator`, `new`, `delete`, and `explicit`, but still leaves the source
  idea's first-wave words such as `typename`, `noexcept`, `nullptr`, `friend`,
  `virtual`, `final`, `override`, access labels, `mutable`, and the
  alternative operator spellings as plain identifiers.
- [x] Parser hot spots currently rely on raw identifier spelling for newly
  reservable words including `typename` in template / qualified-type parsing
  and `final` in record parsing, with additional later follow-ons for
  `noexcept`, `nullptr`, access labels, `friend`, and `virtual`.
- [x] Recorded full-suite baseline in `test_before.log`: `100% tests passed,
  0 tests failed out of 2341`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_typename_final_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_typename_final_parse.cpp)
  and registered it as a parse-only positive case.
- [x] Reserved `typename` and `final` in the lexer as `KwTypename` and
  `KwFinal`, then updated the parser compatibility paths that previously
  expected those spellings as plain identifiers.
- [x] Revalidated the slice with targeted parse coverage:
  `keyword_typename_final_parse`, `qualified_type_spelling_shared_parse`,
  `record_final_specifier_parse`, `template_typename_typed_nttp_parse`, and
  `if_condition_decl_parse`.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2342`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_noexcept_nullptr_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_noexcept_nullptr_parse.cpp)
  and registered it as a parse-only positive case to cover both `noexcept`
  exception-spec parsing and `nullptr` default / expression handling under
  reserved keyword tokens.
- [x] Reserved `noexcept` and `nullptr` in the lexer as `KwNoexcept` and
  `KwNullptr`, then updated the parser compatibility paths that skip exception
  specs and treat `nullptr` as a null-pointer constant in expression and
  typeof-like parsing.
- [x] Revalidated the slice with targeted coverage:
  `keyword_noexcept_nullptr_parse`, `keyword_typename_final_parse`,
  `default_param_value_parse`, `noexcept_decl_parse`, and
  `noexcept_method_parse`, plus a manual `--lex-only` probe confirming
  `KW_noexcept` / `KW_nullptr` output.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2343`.
- [x] Recorded fresh full-suite baseline in `test_before.log` before the next
  slice: `100% tests passed, 0 tests failed out of 2343`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_access_virtual_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_access_virtual_parse.cpp)
  and registered it as a parse-only positive case covering reserved access
  labels plus `virtual` in a record base clause.
- [x] Reserved `public`, `private`, `protected`, and `virtual` in the lexer as
  `KwPublic`, `KwPrivate`, `KwProtected`, and `KwVirtual`, then updated the
  record access-label and base-clause compatibility helpers to accept the new
  token kinds.
- [x] Revalidated the slice with targeted coverage:
  `keyword_access_virtual_parse`, `access_labels_parse`,
  `access_labels_treated_public_runtime`, `record_member_prelude_parse`, and
  `record_base_clause_setup_parse`, plus a manual `--lex-only` probe
  confirming `KW_public`, `KW_protected`, and `KW_virtual` output.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2344`.
- [x] Recorded fresh full-suite baseline in `test_before.log` before the
  `friend` slice: `100% tests passed, 0 tests failed out of 2344`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_friend_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_friend_parse.cpp)
  and registered it as a parse-only positive case covering friend declarations
  and inline friend definitions under a reserved `friend` token.
- [x] Reserved `friend` in the lexer as `KwFriend`, then updated the
  record-member prelude compatibility helper and the top-level C++ declaration
  probe to accept the new token kind.
- [x] Revalidated the slice with targeted coverage:
  `keyword_friend_parse`, `friend_access_parse`,
  `friend_inline_operator_parse`, `record_member_prelude_parse`, and
  `record_member_mixed_prelude_parse`, plus a manual `--lex-only` probe
  confirming `KW_friend` output.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2345`.
- [x] Recorded fresh full-suite baseline in `test_before.log` before the
  `mutable` slice: `100% tests passed, 0 tests failed out of 2345`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_mutable_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_mutable_parse.cpp)
  and registered it as a parse-only positive case covering reserved `mutable`
  use in both record-member declarations and lambda-specifier parsing.
- [x] Reserved `mutable` in the lexer as `KwMutable`, fixed the keyword
  fast-path so `m...` words can classify correctly, then updated parser
  storage-class and lambda-specifier compatibility paths to accept the new
  token kind.
- [x] Revalidated the slice with targeted coverage:
  `keyword_mutable_parse`, `keyword_friend_parse`,
  `record_member_prelude_parse`, `record_member_mixed_prelude_parse`, and
  `eastl_slice7c_struct_body_recovery`, plus a manual `--lex-only` probe
  confirming `KW_mutable` output.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2346`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_and_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_and_parse.cpp),
  registered it as a parse-only positive case, and added explicit lexer / AST
  assertions in
  [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
  to pin `AMP_AMP 'and'` output plus `Function(operator_and)` in the parse
  dump.
- [x] Reserved `and` in the lexer by classifying it to the existing
  `TokenKind::AmpAmp`, then updated overloaded-operator parsing helpers so
  `operator and` lowers through the same `operator_and` path as `operator&&`.
- [x] Revalidated the slice with targeted coverage:
  `keyword_and_parse`, `cpp_lex_keyword_and_tokens`,
  `cpp_parse_keyword_and_operator_dump`, `operator_decl_eq_parse`, and
  `friend_inline_operator_parse`, plus a manual `--lex-only` probe confirming
  `AMP_AMP 'and'` output.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2349`.
- [x] Recorded fresh full-suite baseline in `test_before.log` before the `or`
  slice: `100% tests passed, 0 tests failed out of 2349`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_or_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_or_parse.cpp),
  registered it as a parse-only positive case, and added explicit lexer / AST
  assertions in
  [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
  to pin `PIPE_PIPE 'or'` output plus `Function(operator_or)` in the parse
  dump.
- [x] Reserved `or` in the lexer by classifying it to the existing
  `TokenKind::PipePipe`, then updated overloaded-operator parsing helpers so
  `operator or` lowers through the same `operator_or` path as `operator||`.
- [x] Revalidated the slice with targeted coverage:
  `keyword_or_parse`, `cpp_lex_keyword_or_tokens`,
  `cpp_parse_keyword_or_operator_dump`, `keyword_and_parse`,
  `operator_decl_eq_parse`, and `friend_inline_operator_parse`.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2352`.
- [x] Recorded fresh full-suite baseline in `test_before.log` before the
  `not` slice: `100% tests passed, 0 tests failed out of 2352`.
- [x] Added
  [tests/cpp/internal/postive_case/keyword_not_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/keyword_not_parse.cpp),
  registered it as a parse-only positive case, and added explicit lexer / AST
  assertions in
  [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
  to pin `BANG 'not'` output plus `Function(operator_not)` in the parse dump.
- [x] Reserved `not` in the lexer by classifying it to the existing
  `TokenKind::Bang`, allowing both unary `not expr` parsing and overloaded
  `operator not` lowering through the existing logical-not token path.
- [x] Revalidated the slice with targeted coverage:
  `keyword_not_parse`, `cpp_lex_keyword_not_tokens`,
  `cpp_parse_keyword_not_operator_dump`, `keyword_and_parse`,
  `keyword_or_parse`, `operator_decl_eq_parse`, and
  `friend_inline_operator_parse`.
- [x] Recorded full-suite post-change status in `test_after.log`:
  `100% tests passed, 0 tests failed out of 2355`.

## Next Intended Slice

After `not_eq`, evaluate whether another remaining
alternative-operator spelling to land without widening into a broader alias
batch or forcing unary-expression cleanup beyond the lexer/parser boundary.

## Blockers

- None recorded during activation.

## Resume Notes

- `02_parser_cpp_surface_normalization_plan.md` explicitly calls out lexer
  keyword expansion as an earlier prerequisite, which is why `01` was activated
  instead of `02`.
- `__backend_port_plan.md` is an umbrella roadmap and should not be used as the
  direct active plan without a narrower child idea.
- First implementation slice is intentionally narrow: `typename` and `final`
  only. Leave `noexcept`, `nullptr`, access labels, `friend`, `virtual`, and
  alternative operator spellings for follow-on slices unless they block this
  change.
- The first slice finished cleanly with monotonic full-suite results:
  `2341 -> 2342` total passing tests due to the new committed parse regression.
- Second slice also finished cleanly with monotonic full-suite results:
  `2342 -> 2343` total passing tests due to the new committed parse regression.
- Third slice finished cleanly with monotonic full-suite results:
  `2343 -> 2344` total passing tests due to the new committed parse regression.
- Fourth slice finished cleanly with monotonic full-suite results:
  `2344 -> 2345` total passing tests due to the new committed parse regression.
- Fifth slice finished cleanly with monotonic full-suite results:
  `2345 -> 2346` total passing tests due to the new committed parse regression.
- Fresh baseline before the `and` slice remains clean:
  `100% tests passed, 0 tests failed out of 2346`.
- The `and` slice finished cleanly with monotonic full-suite results:
  `2346 -> 2349` total passing tests due to the new parse case plus explicit
  lexer / AST assertions.
- Fresh baseline before the `or` slice remains clean:
  `100% tests passed, 0 tests failed out of 2349`.
- The `or` slice finished cleanly with monotonic full-suite results:
  `2349 -> 2352` total passing tests due to the new parse case plus explicit
  lexer / AST assertions.
- Fresh baseline before the `not` slice remains clean:
  `100% tests passed, 0 tests failed out of 2352`.
- The `not` slice finished cleanly with monotonic full-suite results:
  `2352 -> 2355` total passing tests due to the new parse case plus explicit
  lexer / AST assertions.
- A direct runtime regression for overloaded `operator&&` exposed a separate
  lowering issue where record-typed operands are compared as scalars in
  condition contexts. That behavior is adjacent but out of scope for this
  lexer/parser runbook, so the committed `and` coverage stays at the
  lexer/parse boundary.
