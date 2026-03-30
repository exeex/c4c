# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-30

## Current Active Item

- Step 2/4: reduce the still-live `std::vector` libc++ frontier after landing
  the `tuple:1044` deduction-guide parser fix; the forcing repro now first
  reports `tuple:1171`

## Todo

- [x] Re-run the direct `std::vector` repro and capture the first surviving
      `libc++` parser errors
- [x] Identify that the apparent `byte.h` / `construct_at.h` lead failures do
      not reproduce in isolation, while `__type_traits/is_unsigned.h` fails
      standalone
- [x] Confirm that `c4cll --pp-only` incorrectly takes the fallback branch of
      `__type_traits/is_unsigned.h` because `__has_builtin(__is_unsigned)`
      expands to `0`
- [x] Add regression coverage for the `__has_builtin(__is_unsigned)` query plus
      a parse-only libc++ header testcase for `__type_traits/is_unsigned.h`
- [x] Implement the smallest fix required by that reduced preprocessor blocker
- [x] Re-run the direct repro to confirm the next `libc++` frontier
- [x] Validate the completed slice with targeted tests plus a monotonic
      full-suite comparison
- [x] Reduce the contextual `max_size_type.h:564` failure to the smallest
      committed internal testcase
- [x] Implement the narrowest parser fix for the reduced `max_size_type`
      frontier
- [x] Re-run the direct `std::vector` repro to confirm the next post-
      `max_size_type` frontier
- [x] Validate the completed `max_size_type` slice with targeted tests plus a
      monotonic full-suite comparison
- [x] Re-run the direct `std::vector` repro after merging `origin/main` and
      confirm `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/__type_traits/common_reference.h:155:59`
      (`expected=LESS got='__apply'`) is still the first surviving libc++
      parser failure
- [x] Reduce the `common_reference.h:155` spelling to a focused internal parser
      testcase for dependent member templates used as template-template
      arguments
- [x] Teach top-level and record-scope `using` alias parsing to accept
      attributes between the alias name and `=`
- [x] Validate the attributed-alias slice with targeted parser coverage plus a
      monotonic full-suite comparison (`before: 706/731`, `after: 707/732`,
      zero new failures)
- [x] Reduce the `__compare/synth_three_way.h:28` failure to a focused
      internal parser testcase for generic lambdas with template parameter
      lists and trailing requires-clauses
- [x] Implement the narrowest parser fix for the reduced `synth_three_way`
      generic-lambda frontier
- [x] Re-run the direct `std::vector` repro to confirm the next post-
      `synth_three_way` frontier
- [x] Validate the completed `synth_three_way` slice with targeted parser
      coverage plus a monotonic full-suite comparison (`before: 710/735`,
      `after: 711/736`, zero new failures)
- [x] Reduce the `__utility/pair.h:463` failure to a focused internal parser
      testcase covering a C++20 `if (init; condition)` statement with a local
      declaration init-clause
- [x] Teach `if` statement parsing to keep declaration conditions working when
      a C++17/C++20 `; condition` tail follows the init declaration
- [x] Re-run the direct `std::vector` repro to confirm the next post-
      `pair.h` frontier
- [x] Validate the completed `pair.h` slice with targeted parser coverage plus
      a monotonic full-suite comparison (`before: 711/736`, `after: 712/737`,
      zero new failures)
- [x] Add a focused internal regression for attributed out-of-class templated
      special-member definitions
- [x] Teach the top-level special-member probe to skip leading GNU/C++11
      attributes plus post-attribute `inline` / `constexpr` / `consteval`
      specifiers before matching qualified constructors/operators
- [x] Validate the attributed special-member parser improvement with targeted
      coverage plus a monotonic full-suite comparison (`before: 712/737`,
      `after: 713/738`, zero new failures)
- [x] Reduce the live `tuple:512` frontier to a focused internal parser
      testcase covering constructor initializer lists with template-id base
      names and pack-expanded calls
- [x] Teach constructor initializer list parsing to accept template-id
      initializer targets such as `leaf<I, Ts>(...)...` in both record-scope
      and top-level constructor definitions
- [x] Re-run the direct `std::vector` repro to confirm the post-`tuple:512`
      frontier moved forward to `tuple:1044`
- [x] Validate the completed `tuple:512` slice with targeted libc++ parser
      coverage plus a monotonic full-suite comparison (`before: 707/733`,
      `after: 714/739`, zero new failures)
- [x] Reduce the live `tuple:1044` frontier to a focused internal parser
      testcase covering deduction guides with template-id parameter types
- [x] Teach top-level parsing to stop misclassifying deduction-guide parameter
      types such as `pair<T1, T2>` as grouped declarators and to consume the
      resulting deduction-guide declaration shape without a parser failure
- [x] Re-run the direct `std::vector` repro to confirm the post-`tuple:1044`
      frontier moved forward to `tuple:1171`
- [x] Validate the completed `tuple:1044` slice with targeted libc++ parser
      coverage plus a monotonic full-suite comparison (`before: 707/733`,
      `after: 715/740`, zero new failures)

## Completed

- [x] Selected `ideas/open/04_std_vector_bringup_plan.md` as the sole
      activation target for the new `libc++` parser branch
- [x] Confirmed there was no active `plan.md` / `plan_todo.md` pair to
      overwrite
- [x] Generated a fresh `plan.md` runbook derived from the `libc++`-updated
      `std::vector` idea
- [x] Verified that existing typed-NTTP SFINAE coverage already passes, so the
      first live blocker is not the raw `__enable_if_t<...>` template-parameter
      shape by itself
- [x] Reduced the first independent libc++ failure to `#include
      <__type_traits/is_unsigned.h>`, which failed because the preprocessor
      missed `__has_builtin(__is_unsigned)`
- [x] Added `tests/cpp/internal/postive_case/libcxx_is_unsigned_header_parse.cpp`
      and taught the preprocessor `__has_builtin` probe to recognize
      `__is_unsigned`
- [x] Added direct parse-only coverage for the builtin trait spelling
      `__is_unsigned(T)` in
      `tests/cpp/internal/postive_case/builtin_trait_is_unsigned_parse.cpp`
- [x] Added non-dependent runtime coverage for `__is_unsigned(unsigned)` and
      `__is_unsigned(int)` in
      `tests/cpp/internal/postive_case/builtin_trait_is_unsigned_runtime.cpp`
- [x] Reduced the next independent libc++ failure to `#include
      <__type_traits/is_signed.h>`, which failed because the preprocessor
      missed `__has_builtin(__is_signed)`
- [x] Added direct parse-only and runtime coverage for `__is_signed(T)` plus a
      libc++ header regression for `__type_traits/is_signed.h`
- [x] Added frontend coverage for builtin transform traits inside
      `__is_same(...)`, including `__remove_cv`, `__remove_cvref`, and
      `__remove_reference_t`
- [x] Taught type parsing to accept builtin transform-trait spellings as
      type-producing operands in type context
- [x] Re-ran the direct `std::vector` repro and confirmed the old
      `byte.h` / `construct_at.h` / `is_unsigned` frontier disappeared, exposing
      later parser failures in `__algorithm/unwrap_iter.h`,
      `__ranges/size.h`, `__algorithm/comp_ref_type.h`, and `__utility/pair.h`
- [x] Re-ran the direct `std::vector` repro after the `__is_signed` slice and
      confirmed the old `__algorithm/unwrap_iter.h:63` frontier disappeared,
      leaving `/usr/include/c++/v1/__ranges/size.h:71` as the first surviving
      libc++ parser error
- [x] Confirmed `#include <__iterator/iterator_traits.h>` now gets past the
      earlier EOF / unbalanced-state failure and instead exposes more localized
      parser frontiers in `__type_traits/is_void.h` and
      `__type_traits/common_reference.h`
- [x] Re-ran the direct `std::vector` repro after the transform-trait slice and
      confirmed the old `__type_traits/is_void.h:26` failure disappeared,
      leaving `/usr/include/c++/v1/__type_traits/common_reference.h:155` as the
      first surviving libc++ parser error
- [x] Merged the latest `origin/main` parser work into this branch so the next
      libc++ reduction happens on top of the current upstream baseline
- [x] Added
      `tests/cpp/internal/postive_case/dependent_member_template_template_arg_parse.cpp`
      to cover attributed dependent member templates used as
      template-template arguments in type context
- [x] Taught both top-level and record-scope `using` alias parsing to accept
      `[[...]]` attributes between the alias name and `=`
- [x] Reduced the post-merge `std::vector` frontier at
      `__utility/integer_sequence.h:82` to a focused internal testcase for
      dependent member-template operator calls inside a fold expression
- [x] Taught postfix expression parsing to accept `.template` / `->template`
      member access with explicit template arguments before the final call
- [x] Taught comma-expression parsing to accept the `(expr, ...)` fold form
      used by `__for_each_index_sequence`
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `__utility/integer_sequence.h:82` frontier disappeared, exposing
      `__compare/ordering.h:44`,
      `__compare/common_comparison_category.h:42`, and later `__utility/pair.h`
      parser failures instead
- [x] Validated the completed `integer_sequence` slice with targeted parser
      coverage for the reduced testcase, the earlier dependent-member-template
      testcase, and the libc++ signed/unsigned header probes
- [x] Reduced the `__compare/ordering.h:44` constructor failure to a focused
      internal parser testcase covering a templated record constructor with a
      leading GNU attribute before the constructor name
- [x] Taught record-constructor detection to preserve constructor parsing when
      leading GNU or `[[...]]` attributes appear before the constructor name
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `__compare/ordering.h:44` frontier disappeared, leaving
      `__compare/common_comparison_category.h:42` as the new first surviving
      libc++ parser error
- [x] Validated the completed `ordering.h` slice with targeted parser coverage
      plus a monotonic full-suite comparison (`before: 707/733`, `after:
      709/734`, zero new failures)
- [x] Reduced the `__compare/common_comparison_category.h:42` failure to a
      focused internal parser testcase covering a top-level array-reference
      parameter of an injected enum type
- [x] Registered C++ enum definitions as injected type names so later
      declarators treat `E` as a type head instead of a parameter name
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `__compare/common_comparison_category.h:42` frontier disappeared,
      exposing `__compare/synth_three_way.h:28` as the new first surviving
      libc++ parser error
- [x] Validated the completed `common_comparison_category` slice with targeted
      parser coverage and a fresh full-suite snapshot that matches the last
      recorded branch baseline (`709/734` passing)
- [x] Added
      `tests/cpp/internal/postive_case/libcxx_synth_three_way_generic_lambda_parse.cpp`
      to cover the reduced `synth_three_way` generic-lambda spelling directly
- [x] Taught lambda placeholder parsing to skip optional generic template
      heads and trailing requires-clauses before looking for the lambda body
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `__compare/synth_three_way.h:28` frontier disappeared, exposing
      `__utility/pair.h:463` and later `__iterator/advance.h:160` parser
      failures instead
- [x] Validated the completed `synth_three_way` slice with targeted parser
      coverage for the reduced testcase, the earlier requires-expression
      testcase, and the existing lambda parse coverage
- [x] Added
      `tests/cpp/internal/postive_case/if_init_statement_decl_parse.cpp`
      to cover the reduced `pair.h` C++20 `if (init; condition)` spelling
      directly
- [x] Taught `if` statement parsing to accept declaration init-clauses
      followed by a semicolon and trailing condition expression while
      preserving the existing scoped declaration model
- [x] Re-ran the direct `std::vector` repro and confirmed the
      `__utility/pair.h:463` frontier disappeared, exposing
      `__vector/vector_bool.h:737`, `tuple:512`, `tuple:1044`, `tuple:1171`,
      and later `typename`-expression follow-ons instead
- [x] Validated the completed `pair.h` slice with targeted parser coverage for
      the reduced testcase, the existing `if_condition_decl` testcase, the
      earlier `synth_three_way` testcase, and the libc++ signed/unsigned
      header probes

## Next Intended Slice

- Reduce the new first surviving `tuple:1171:123` failure to a focused
  internal parser testcase for the unexpected `...` expression spelling now
  leading the libc++ `std::vector` repro.
- Use parser-debug traces around `tuple:1171` to determine whether the live
  failure is a fold-expression-local gap or residual parse-state loss from
  earlier tuple declarations.
- Re-run the forcing `std::vector` repro after the next reduction and confirm
  whether `tuple:1378`, `tuple:1416`, and
  `__memory_resource/polymorphic_allocator.h:132` remain the immediate
  follow-ons once `tuple:1171` is genuinely cleared.

## Blockers

- The latest direct repro no longer leads with either
  `common_reference.h:155`, `integer_sequence.h:82`, or
  `__compare/ordering.h:44`; it no longer first fails in `__utility/pair.h`.
- The latest direct repro no longer fails first at `tuple:512`; it now first
  fails at `tuple:1171:123` with `unexpected token in expression: ...`,
  followed by `tuple:1378`, `tuple:1416`, and
  `__memory_resource/polymorphic_allocator.h:132` `typename`-expression
  follow-ons.
- The new reduced testcase
  `tests/cpp/internal/postive_case/libcxx_tuple_ctor_base_template_init_parse.cpp`
  covers the cleared `tuple:512` constructor-initializer gap directly.
- The new reduced testcase
  `tests/cpp/internal/postive_case/libcxx_tuple_pair_deduction_guide_parse.cpp`
  covers the cleared `tuple:1044` deduction-guide gap directly.
- Simple standalone tuple deduction-guide spellings were already being dropped
  as `Empty` nodes; the actual `tuple:1044` blocker was that grouped-declarator
  probing treated `tuple(pair<T1, T2>)` as if `pair` were a parenthesized
  declarator name and then failed on `<`.
- The current full-suite run still contains unrelated environment/path failures
  such as missing `/workspaces/c4c/...` fixtures and unavailable GNU
  `bits/...` headers, so those results are not attributable to this patch.
- Older GNU `libstdc++` blocker ordering in the source idea remains useful as
  history, but it is not the active execution frontier for this branch.

## Resume Notes

- Treat the 2026-03-30 `libc++` note in the source idea as the authoritative
  frontier for this branch.
- Keep the plan narrow to one live parser blocker at a time.
- This slice showed the first visible libc++ failure was actually a
  preprocessor builtin-query mismatch, not a raw template-parameter parser bug.
- `__is_unsigned` now has direct parse-only coverage plus concrete runtime
  coverage; dependent template-value evaluation for builtin traits is still a
  separate follow-on concern if libc++ reduction reaches it.
- `__is_signed` now has the same direct parse-only/runtime/header coverage
  pattern, and clearing its builtin branch was enough to move the libc++ path
  past the earlier `unwrap_iter` frontier.
- Builtin transform traits now work in `__is_same(...)` type-argument
  positions, which was enough to clear the `is_void.h` blocker and advance the
  libc++ path to `common_reference.h`.
- `origin/main` brought in additional parser fixes and regressions from the GNU
  bring-up branch; keep them as upstream baseline improvements, but do not let
  them widen this branch's active plan beyond the current libc++ frontier.
- Attributed `using` aliases now parse in both top-level and record scope when
  `[[...]]` appears between the alias name and `=`.
- Constructor initializer lists now accept template-id initializer targets with
  pack expansion, which was enough to clear the libc++ `tuple:512` frontier.
- The isolated reduced testcase for `__xref<T>::template apply` still passes,
  and the subsequent `integer_sequence.h` dependent-member-template frontier is
  now cleared.
- Named C++ enums are now registered as injected type names, which lets
  top-level declarators parse `const E (&param)[N]` the same way record members
  already handled equivalent array-reference parameters.
- `tests/cpp/internal/postive_case/dependent_member_template_operator_call_parse.cpp`
  now covers the reduced `integer_sequence` spelling directly.
- `tests/cpp/internal/postive_case/libcxx_ordering_hidden_consteval_ctor_parse.cpp`
  now covers the reduced `ordering.h` spelling directly.
- `tests/cpp/internal/postive_case/libcxx_common_comparison_category_array_param_parse.cpp`
  now covers the reduced `common_comparison_category` spelling directly.
- `tests/cpp/internal/postive_case/libcxx_synth_three_way_generic_lambda_parse.cpp`
  now covers the reduced `synth_three_way` generic-lambda spelling directly.
- `tests/cpp/internal/postive_case/if_init_statement_decl_parse.cpp` now
  covers the reduced `pair.h` C++20 `if (init; condition)` spelling directly.
- `tests/cpp/internal/postive_case/libcxx_vector_bool_move_ctor_parse.cpp` now
  covers attributed out-of-class templated special-member definitions, and the
  parser now correctly keeps that reduced case on the `Vec::Vec` constructor
  path instead of misclassifying it as a free function.
- A direct parse of `#include <__utility/integer_sequence.h>` now succeeds; the
  remaining libc++ failures are later headers surfaced by the forcing
  `std::vector` include stack.
- The `ordering.h` slice only needed constructor-member prelude handling for
  leading GNU / C++11 attributes; trailing `__enable_if__` attributes already
  parsed once the member was classified as a constructor.
- Lambda placeholder parsing now skips optional generic template heads and
  trailing requires-clauses before looking for the lambda body.
- `if` statement parsing now accepts a declaration init-clause followed by a
  semicolon and trailing condition expression, while preserving the existing
  declaration-condition block scoping model.
- Prefer shared parser or preprocessor compatibility fixes over libc++-specific
  hacks.
- Do not reactivate backend umbrella work while this parser bring-up is active.
