# Idea 99 Step 8 Pre-Demotion Route Review

## Scope

Objective: review idea 99 implementation through Step 7 before any Step 8
legacy fallback demotion.

Focus: HIR module function/global structured lookup mirror work from activation
commit `d7e60821` through `HEAD`, especially `src/frontend/hir/hir_ir.hpp`,
HIR lowering/indexing files, `src/apps/c4cll.cpp`, and the new HIR test case.

I used direct diff review. `c4c-clang-tools` was not needed because the touched
surface is small and the relevant lookup/control-flow paths were directly
visible in the diff.

## Review Base

Chosen base: `d7e60821` (`[plan+idea] Activate HIR module structured lookup mirror plan`).

Reason: this is the unambiguous activation commit for the current active plan
and source idea. `git log --oneline --extended-regexp --grep='\[[^]]*plan[^]]*\]' -- plan.md todo.md ideas/open/`
shows no later plan checkpoint for idea 99.

Commits since base: 7.

Reviewed commits:

- `66ff0b3a add hir module declaration lookup key`
- `b4cf2894 add hir declaration name text metadata`
- `34f03ee0 populate hir declaration name text metadata`
- `ff4b45bb dual-write hir module structured mirrors`
- `275ae806 use hir structured decl lookup helpers`
- `db55921e record hir structured lookup parity`
- `46846e64 cover hir structured module lookup parity`

## Findings

### Medium: Step 8 fallback demotion is not yet supported by the proof

The implementation adds structured maps and resolver comparison, but the current
test/proof does not establish that rendered fallback can be safely demoted.

Evidence:

- `Module::resolve_function_decl` still returns `LinkNameId` matches before
  structured lookup, and only compares structured vs legacy after that bridge
  misses (`src/frontend/hir/hir_ir.hpp:1209`).
- `Module::resolve_global_decl` still returns concrete `GlobalId` and
  `LinkNameId` matches before structured lookup, and only compares structured
  vs legacy after those bridges miss (`src/frontend/hir/hir_ir.hpp:1277`).
- Normal variable lowering can set `DeclRef::global` from the rendered
  `global_index` before resolver parity can observe the reference
  (`src/frontend/hir/impl/expr/scalar_control.cpp:184`).
- Direct-call helper paths still seed call references from rendered names and
  link-name lookup (`src/frontend/hir/impl/expr/call.cpp:35`,
  `src/frontend/hir/impl/expr/call.cpp:46`).
- The new test asserts rendered HIR shape and absence of a dumped parity
  mismatch section (`tests/cpp/internal/hir_case/CMakeLists.txt:327`), but it
  does not prove that fallback-dependent cases continue to resolve when rendered
  lookup is demoted.

This is not a behavior regression through Step 7. It is a Step 8 readiness
blocker: rendered fallback demotion should not proceed until there is targeted
proof that structured lookup is authoritative for the intended cases, while
concrete ID and `LinkNameId` precedence remain explicitly preserved.

## Positive Observations

- No testcase-overfit implementation pattern found. The diff adds structured
  key/index/helper machinery rather than named-case matching.
- Existing rendered-name maps are preserved and dual-written through
  `Module::index_function_decl` and `Module::index_global_decl`
  (`src/frontend/hir/hir_ir.hpp:1071`, `src/frontend/hir/hir_ir.hpp:1078`).
- Resolver mismatch handling is behavior-preserving during the proof window:
  when structured and legacy disagree, the resolver records a mismatch and
  returns legacy (`src/frontend/hir/hir_ir.hpp:1214`,
  `src/frontend/hir/hir_ir.hpp:1285`).
- HIR dump visibility for observed mismatches is present
  (`src/frontend/hir/impl/inspect/printer.cpp:164`).
- The focused HIR test is appropriate Step 7 coverage for the namespace-qualified
  rendered output shape and for ensuring no observed mismatch is printed.

## Validation Reviewed

`test_after.log` records a passing command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`

The log shows 72/72 HIR-labeled tests passed, including:

- `cpp_hir_template_global_specialization`
- `cpp_hir_if_constexpr_branch_unlocks_later`
- `cpp_hir_multistage_shape_chain`
- `cpp_hir_module_decl_lookup_structured_mirror`

This is sufficient for Step 7 route review, but not sufficient to justify Step
8 rendered fallback demotion.

## Judgments

Plan alignment: `on track` through Step 7.

Idea alignment: `matches source idea`.

Technical-debt judgment: `watch`.

Validation sufficiency: `needs broader proof` before Step 8 fallback demotion.

Reviewer recommendation: `narrow next packet`.

Recommended next packet: keep rendered fallback parked and add targeted proof or
instrumentation that distinguishes structured hits, legacy hits, concrete-ID
hits, and `LinkNameId` hits for module function/global references. Only consider
fallback demotion after that evidence is green and supervisor approves the
regression state.
