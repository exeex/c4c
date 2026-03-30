# Active Plan Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-30

## Current Active Item

- Step 4: re-run the direct `libc++` `std::vector` repro after merging
  `origin/main`, then reduce the first surviving parser frontier beyond
  `__type_traits/common_reference.h`

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

## Next Intended Slice

- Re-run the direct `std::vector` repro on top of the merged tree and confirm
  whether `/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1/__type_traits/common_reference.h:155:59`
  (`parse_top_level expected=LESS got='__apply'`) is still the first surviving
  parser failure.
- If it is, reduce that frontier into one internal parse testcase before
  changing parser code.

## Blockers

- The next live frontier is now in later libc++ parser surfaces such as
  `__type_traits/common_reference.h`, `__ranges/size.h`, and
  `__algorithm/comp_ref_type.h`, but they are not reduced yet.
- The fresh `origin/main` merge may have shifted the exact first surviving
  libc++ failure, so the repro ordering needs to be reconfirmed before the next
  parser edit.
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
- Prefer shared parser or preprocessor compatibility fixes over libc++-specific
  hacks.
- Do not reactivate backend umbrella work while this parser bring-up is active.
