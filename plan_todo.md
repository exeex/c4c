# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/04_std_vector_bringup_plan.md
Source Plan: plan.md
Last Updated: 2026-03-29

## Active Item

- Step 3: identify the first surviving parser corruption point after the
  namespaced-concept recovery fix

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

## Next Slice

- reduce the new `stl_iterator.h` `else` failures to a standalone parser repro
- inspect whether the new frontier is another recovery leak around constrained
  member/operator parsing or an `if constexpr` / requires-body mismatch
- only revisit `functional_hash.h` / `stl_bvector.h` if they remain after the
  new earlier blocker is fixed

## Notes

- user-reported current evidence indicates `functional_hash.h:54` is likely a
  downstream symptom, not root cause
- focus on parser-state drift induced by header interaction
- avoid semantic trait implementation unless reduction proves it is required
- the concept fix improved the active `<vector>` path but did not finish the
  bring-up; remaining failures are now later and more localized

## Blockers

- none yet
