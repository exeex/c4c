# Plan Todo

Status: Active
Source Idea: ideas/open/01_cpp17_keyword_support_plan.md
Source Plan: plan.md

## Active Item

- [ ] Step 2: choose the next narrow C++17 keyword slice after `typename` /
  `final`, likely `noexcept` and/or `nullptr`, and add the matching lexer /
  parser coverage without widening scope

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
- [ ] Add the narrowest validating lexer or parser test for the next slice
- [ ] Implement the next keyword-classification slice
- [ ] Re-run targeted tests, nearby coverage, and the full suite

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

## Next Intended Slice

Audit the smallest remaining parser-sensitive keyword pair, likely `noexcept`
and `nullptr`, because both still appear in lexer-gap inventory and already
have nearby parser spelling checks that should convert cleanly to token-aware
handling.

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
