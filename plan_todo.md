# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: reduce and fix the first surviving `stl_iterator.h` parser corruption
  point after the namespaced-concept recovery fix

## Completed

- re-read the parked source idea and confirmed this slice belongs to the
  `std::vector` bring-up plan, not the separate builtin-trait audit
- re-activated the parked `std::vector` idea into `plan.md`
- reduced the first failing `<vector>` top-level prefix to
  `<bits/stl_algobase.h>` + `<bits/stl_bvector.h>`
- further reduced that interaction to `<bits/iterator_concepts.h>` and then to
  a smaller namespaced `concept ... = requires(...) { ... };` shape
- confirmed `concept` was not lexed as a keyword
- added `KwConcept` and a balanced top-level concept-declaration recovery path
  so `requires { ... ; ... }` bodies no longer truncate at the first inner `;`
- added and passed
  `tests/cpp/internal/postive_case/cpp20_namespaced_concept_requires_parse.cpp`
- re-ran `tests/cpp/std/std_vector_simple.cpp` and confirmed the `<vector>`
  frontier moved forward: the first errors now start at
  `/usr/include/c++/14/bits/stl_iterator.h:2063` and `:2240` on unexpected
  `else`
- triaged the follow-on negative-test fallout from the concept slice:
  - restored `cpp20_requires_clause_preserves_following_decl` by treating
    `static_assert` as a declaration boundary after a top-level requires-clause
  - retired obsolete parser-debug negative cases that now conflict with newer
    positive parser support for attributed params / constrained member parsing
- converted `static_assert` from parser-only swallowing into a real AST node
  (`NK_STATIC_ASSERT`) shared by top-level, statement, and record-member paths
- taught sema to evaluate non-dependent `static_assert` conditions centrally and
  report false / non-constant conditions there instead of in scattered parser
  branches
- taught HIR lowering to treat `NK_STATIC_ASSERT` as a no-op statement after
  sema validation
- added targeted C++ coverage:
  - positive: `tests/cpp/internal/postive_case/static_assert_runtime.cpp`
  - negative: `tests/cpp/internal/negative_case/static_assert_record_member_false.cpp`
- targeted ctest rerun passed for:
  - `cpp_negative_tests_cpp20_requires_clause_preserves_following_decl`
  - `cpp_negative_tests_static_assert_record_member_false`
  - `cpp_positive_sema_static_assert_runtime_cpp`

## Next Slice

- reduce the new `stl_iterator.h` `else` failures to a standalone parser repro
- inspect whether the new frontier is another recovery leak around constrained
  member/operator parsing or an `if constexpr` / requires-body mismatch
- only revisit `functional_hash.h` / `stl_bvector.h` if they remain after the
  new earlier blocker is fixed
- future static_assert follow-up if needed:
  template-dependent checks inside deferred template bodies are still not fully
  routed through compile-time instantiation / engine-owned evaluation
- current suspicion:
  `iterator_concepts.h` is still leaving parser state corrupted after the
  header finishes; this no longer looks like only a missing concept-name lookup
  and more like a top-level / record-body recovery leak that causes the next
  struct or template declaration to be swallowed or partially reinterpreted
- next step:
  instrument and reduce the exact `iterator_concepts.h` tail declaration that
  causes the following top-level `struct hash_base` to disappear, then fix the
  record-body or top-level handoff so post-header declarations stay inside the
  correct scope

## Notes

- user-reported current evidence indicates `functional_hash.h:54` is likely a
  downstream symptom, not root cause
- focus on parser-state drift induced by header interaction
- avoid semantic trait implementation unless reduction proves it is required
- the concept fix improved the active `<vector>` path but did not finish the
  bring-up; remaining failures are now later and more localized
- targeted negative-test rerun now passes for the surviving enforced case
- current `static_assert` behavior is now centralized for non-dependent cases,
  but dependent template-body enforcement is still a later semantic slice
- sema still needed a follow-up for `static_assert(consteval_call(...))`; add
  targeted positive/negative coverage once the consteval path is wired in
- reduced the new `stl_iterator.h:2063/:2240` `else` frontier to a standalone
  pseudo-destructor shape: `if (...) obj.~Type(); else ...`
- fixed the pseudo-destructor postfix parse so unbraced `if (...) obj.~Type();
  else ...` no longer mis-associates the `else`
- added and passed
  `tests/cpp/internal/postive_case/pseudo_destructor_if_else_parse.cpp`
- re-ran `tests/cpp/std/std_vector_simple.cpp` and confirmed the
  `stl_iterator.h` `else` frontier is gone; the first remaining error is back at
  `/usr/include/c++/14/bits/functional_hash.h:54`
- reduced the new `functional_hash.h:54` front to
  `#include <bits/iterator_concepts.h>` followed by a templated `hash_base`
  with two member typedefs; the second typedef was leaking to top level
- replaced `concept` declaration skip-to-`;` recovery with a structured
  `concept name = constraint-expression ;` parse that reuses the existing
  expression / requires-expression parser
- started splitting concept names out from typedef/type lookup by adding a
  dedicated parser-side concept symbol registry (`concept_names_`) with
  namespace-aware lookup helpers
- current evidence suggests the remaining `iterator_concepts.h` interaction is
  not only a concept-name lookup issue: after that header, a following
  top-level struct/template can disappear or have only its later members leak to
  top level, pointing to a record-body or top-level recovery state leak

## Blockers

- none yet
