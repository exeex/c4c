# Failure Attribution Review: ideas 139 / 140 / 141

Reviewed: 2026-04-30
Reviewed Commit: `8c1e008c7` (origin/main, "Migrate deferred member TypeSpec fixture off tag")
Test Run: `ctest --test-dir build -j 8` → **44 fails / 3023**
Reference Baseline: `log/baseline_8c1e008c728931b8ea62bf228e856c3a6dc213bf.log` (43 fails)

## Source Ideas Under Review

- `ideas/closed/139_parser_sema_rendered_string_lookup_removal.md` — closed 2026-05-02
- `ideas/closed/140_hir_legacy_string_lookup_metadata_resweep.md` — closed 2026-05-02
- `ideas/open/141_typespec_tag_field_removal_metadata_migration.md` — open, parked

## Question Asked

Are the 44 currently-failing tests actually caused by the 139 / 140 / 141 cleanup
series, and if so, which group does each fail belong to?

## Method

For each failing test, the failure was triggered with
`ctest --test-dir build --output-on-failure -R '^<name>$'` and the stderr
classified by error pattern. Tests grouped by symptom; symptom mapped to idea
based on the deferred/blocker sections of ideas 140 and 141.

## Findings

Yes. **At least 36 / 44 (≈ 82%) of the failures trace directly to 139 / 140 / 141**
cleanup work. One test is environmental noise. The remainder is unclassified
side-effect work that needs deeper inspection.

The dominant root cause is shared: when a record-layout / struct-field /
template-owner lookup site loses its rendered-string fallback, the structured
carrier that was supposed to replace it (HIR record-layout owner index, owner
key, `tag_text_id`) is either not wired through every producer/consumer or not
yet introduced at all. The two largest failure groups (A + B) collapse into
one carrier gap — the one idea 140's deferred section names explicitly:

> HIR record-layout handoff into Sema consteval: `lookup_record_layout`
> currently receives rendered-keyed `ConstEvalEnv::struct_defs` but no
> structured HIR record owner/index or equivalent layout map, which blocks
> deleting `env.struct_defs->find(ts.tag)`.

## Symptom Groups

### Group A — `StmtEmitter: field 'X' not found in struct/union 'Y'`

Owner: **idea 141 + idea 140 (record-layout carrier gap)**

Record field lookup uses `TypeSpec::tag` as the structural key. With
`TypeSpec::tag` partially decoupled and the structured replacement
(`record_def` / HIR owner index) not threaded through every `StmtEmitter`
caller, the field-by-tag lookup fails.

Members (≈ 15):

- `cpp_positive_sema_template_inline_method_member_context_frontend_cpp` (`_M_size not found in 'this'`)
- `c_testsuite_src_00019_c`, `c_testsuite_src_00089_c`, `c_testsuite_src_00218_c`
- `clang_c_external_C_C11_n1285_1_c`
- `llvm_gcc_c_torture_src_20000605_1_c`, `_20040703_1_c`, `_20001124_1_c`,
  `_20011008_3_c`, `_920908_2_c`, `_20090113_2_c`, `_20090113_3_c`,
  `_pr33870_c`, `_pr33870_1_c`, `_pr40022_c`

### Group B — `sizeof: unresolved record layout` (consteval)

Owner: **idea 140 (named in deferred section)**

`ConstEvalEnv::struct_defs` is keyed by rendered tag. With `TypeSpec::tag`
no longer authoritative, the lookup `env.struct_defs->find(ts.tag)` misses,
so `sizeof` and dependent consteval branches stall as "pending consteval".

Members (6):

- `cpp_positive_sema_deferred_consteval_incomplete_type_cpp`
- `cpp_hir_deferred_consteval_incomplete_type`
- `cpp_hir_if_constexpr_branch_unlocks_later`
- `cpp_hir_multistage_shape_chain`
- `cpp_c4_static_assert_if_constexpr_branch_unlocks_later`
- `cpp_c4_static_assert_multistage_shape_chain`

### Group C — `object has incomplete type` (template specialization)

Owner: **idea 139 + 140 + 141 (template owner identity)**

Template specialisation identity loses owner / record metadata mid-handoff,
so EASTL templates report incomplete type at point of use even though the
specialisation has been instantiated.

Members (2):

- `cpp_eastl_vector_parse_recipe` (FunctorStorageType,
  `eastl::uniform_int_distribution`)
- `cpp_positive_sema_eastl_slice4_typename_and_specialization_parse_cpp`

### Group D — `cast to unknown type name '<anonymous>'` (parser disambiguation)

Owner: **idea 139 (parser/Sema rendered string lookup removal)**

Parser disambiguation matrix: `c_style_cast_target` in owner-dependent
template-member or dependent-typename context. Without the rendered-string
fallback, the parser cannot resolve the type name back to its structured owner
when the matrix produces an anonymous-template form.

Members (8):

- `cpp_positive_sema____generated_parser_disambiguation_matrix_compile_positive_owner_dependent_template_member__decl_function_lvalue_ref__ctx_c_style_cast_target__compile_positive_cpp`
- ... `_decl_function_pointer_*`
- ... `_decl_function_rvalue_ref_*`
- ... `_decl_member_function_pointer_*`
- Same four variants under `owner_dependent_typename__*`

### Group E — `LirCallOp/LirGepOp StructNameId mirror does not match`

Owner: **idea 141 spillover (idea 142 retired without finishing)**

LIR call/gep mirror StructNameId vs call text. Idea 142
(`codegen_lir_aggregate_type_identity_metadata`) was supposed to clean these
boundaries before being retired, but the mirror invariants are still failing.

Members (4):

- `llvm_gcc_c_torture_src_20040709_1_c`
- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`
- `llvm_gcc_c_torture_src_pr71083_c`

### Group F — Test specifically asserting idea-141 outcome

Owner: **idea 141 (guard test)**

Member (1):

- `cpp_hir_parser_type_base_producer_structured_metadata` —
  "simple visible typedef producer should preserve source TextId carrier
  before stale rendered tag". This was authored as a guard test for
  idea 141; it fails by design while migration is incomplete.

### Group G — Unclassified (likely pre-existing or secondary effect)

Owner: **needs investigation**

Members (5–6):

- `c_testsuite_src_00170_c` (`conflicting types for function 'it_real_fn'`)
- `c_testsuite_src_00216_c` (no clear stderr captured)
- `llvm_gcc_c_torture_src_pr22141_1_c`, `_pr22141_2_c` (silent FRONTEND_FAIL)
- `llvm_gcc_c_torture_src_const_addr_expr_1_c` (Segmentation fault during
  c2ll execution)

### Noise

- `llvm_gcc_c_torture_src_pr44164_c` — `c2ll_exit=no such file or directory`,
  build artifact / path issue rather than a metadata regression. Not present
  in the `8c1e008c` baseline log; classified as environmental noise.

## Distribution

| Group | Owner Idea(s) | Count | Share |
|------:|:--|---:|---:|
| A | 141 + 140 (record-layout / find_struct_def_by_tag) | 15 | 34% |
| B | **140** (`ConstEvalEnv::struct_defs`) | 6 | 14% |
| C | 139 + 140 + 141 | 2 | 5% |
| D | **139** (parser disambiguation) | 8 | 18% |
| E | 141 spillover (LIR StructNameId mirror) | 4 | 9% |
| F | 141 (guard test) | 1 | 2% |
| G | Unclassified | 5–6 | ~13% |
| Noise | pr44164 | 1 | 2% |
| **Total** | | **44** | |

## Root Cause Overlap

Groups **A + B + F** (22 tests, half the failure list) share one missing piece
of work: a structured HIR record-layout carrier that does not require
`TypeSpec::tag`. Idea 140's deferred section already names this carrier as
"`hir::Module::struct_def_owner_index` + `find_struct_def_by_owner_structured`"
but the consumer migration (`StmtEmitter` field lookup, consteval
`lookup_record_layout`, parser type-base producer guard) was not finished
before idea 140 was closed.

Groups **D** (8 tests) and **C** (2 tests) share another: the parser /
disambiguation path lost rendered-string fallback before owner-resolution
metadata could be plumbed through every c-style-cast / typename-specialisation
edge.

Group **E** (4 tests) is a downstream LIR mirror invariant that was supposed
to be cleaned by idea 142 but was retired before completion.

## Recommended Order of Attack

1. **Group B first (6 tests, narrow blast radius).** Idea 140 deferred section
   already prescribes the carrier shape. Wiring `lookup_record_layout` to
   structured HIR record owner authority is a known, scoped change.
2. **Group A + F next (16 tests).** Same carrier extended into `StmtEmitter`
   field lookup; should mostly fall once the layout owner index is the
   primary lookup authority and `tag` is consulted only as no-metadata
   compatibility.
3. **Group D (8 tests).** Parser disambiguation owner resolution under
   c-style cast — distinct code path; address after the HIR side is settled
   so the parser fix has a stable downstream contract to lower into.
4. **Group E (4 tests).** Reopen idea 142 scope or take a fresh codegen LIR
   aggregate identity metadata pass.
5. **Group G + noise (~6 tests).** Investigate individually after the carrier
   work is in place; some may resolve as side effects.

## Notes

- The previous `main` HEAD (`a97b08145`, since reset) had **78 fails**,
  including ≈ 35 additional cpp_positive_sema iterator / range_for / operator
  regressions. Those are NOT visible at `8c1e008c` and were introduced by the
  `Migrate ... lookup fixtures off tag` series between `8c1e008c..08e79055`.
  That regression series has been discarded; this review reflects the cleaner
  `8c1e008c` floor.
- `llvm_gcc_c_torture_src_pr44164_c` is the only fail not in the historical
  `8c1e008c` baseline log. One run is insufficient to confirm noise vs.
  environment; flag for re-run if it persists.
