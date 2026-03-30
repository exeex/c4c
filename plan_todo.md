# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-30

## Current Active Item

- Step 2/4: reduce the new first surviving libc++ frontier at
  `__compare/common_comparison_category.h:42`, now that the direct
  `std::vector` repro gets past `__compare/ordering.h`

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

## Next Intended Slice

- Reproduce and reduce the new first surviving libc++ frontier at
-  `__compare/common_comparison_category.h:42:59`, where top-level parameter
  parsing now stops at `[` inside the `_ClassifyCompCategory (&__types)[_Size]`
  array-reference parameter.
- Keep the next reduction scoped to the comparison-category headers that became
  visible only after clearing `ordering.h`.
- Re-run the forcing `std::vector` repro after the next parser fix and confirm
  whether `__compare/synth_three_way.h:28` or `__utility/pair.h:463` becomes
  the next lead blocker.

## Blockers

- The latest direct repro no longer leads with either
  `common_reference.h:155`, `integer_sequence.h:82`, or
  `__compare/ordering.h:44`; it now first fails in the `__compare` headers at
  `__compare/common_comparison_category.h:42`.
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
- The isolated reduced testcase for `__xref<T>::template apply` still passes,
  and the subsequent `integer_sequence.h` dependent-member-template frontier is
  now cleared.
- `tests/cpp/internal/postive_case/dependent_member_template_operator_call_parse.cpp`
  now covers the reduced `integer_sequence` spelling directly.
- `tests/cpp/internal/postive_case/libcxx_ordering_hidden_consteval_ctor_parse.cpp`
  now covers the reduced `ordering.h` spelling directly.
- A direct parse of `#include <__utility/integer_sequence.h>` now succeeds; the
  remaining libc++ failures are later headers surfaced by the forcing
  `std::vector` include stack.
- The `ordering.h` slice only needed constructor-member prelude handling for
  leading GNU / C++11 attributes; trailing `__enable_if__` attributes already
  parsed once the member was classified as a constructor.
- Prefer shared parser or preprocessor compatibility fixes over libc++-specific
  hacks.
- Do not reactivate backend umbrella work while this parser bring-up is active.
