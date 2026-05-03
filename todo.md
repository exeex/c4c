# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/base.cpp` member typedef, field, method return,
and method parameter substitution loops around the deletion-probe residual
family that started at line 5942. The concrete template-record clone path now
substitutes direct `TB_TYPEDEF` member types by structured template parameter
metadata (`template_param_text_id`/`tag_text_id`) before using the explicit
no-metadata rendered-tag compatibility fallback, while preserving outer pointer,
reference, cv, and array declarator shape. The packet added
`cpp_hir_parser_type_base_member_substitution_structured_metadata`, which
forces stale rendered `U` tags onto `Box<T, U>` member typedef, field, method
return, and method parameter types and proves `Box<int, double>` still
substitutes all four through structured `T` metadata.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/base.cpp:6188`, after the targeted member
typedef/field/method substitution family.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Classify each deletion-probe failure as parser/HIR-owned,
  compatibility/display/final spelling, or downstream metadata gap.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- The first proof attempt exposed an array-shape regression in this packet's
  substitution helper; the helper now preserves outer array metadata and the
  delegated subset is green.
- The deletion probe log for this packet is
  `/tmp/c4c_typespec_tag_deletion_probe_step4_member_substitution.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 109 of 109
delegated tests, including the new
`cpp_hir_parser_type_base_member_substitution_structured_metadata` test, the
existing parser type-base structured metadata tests, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=108 failed=0 total=108`
before and `passed=109 failed=0 total=109` after. There are no new failing
tests; the pass-count increase over the current baseline is the new parser
type-base member-substitution structured-metadata test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_member_substitution.log`. The
first emitted `base.cpp` residual is now
`src/frontend/parser/impl/types/base.cpp:6188`, outside this packet's owned
member typedef, field, method return, and method parameter substitution family,
so the targeted `base.cpp:5942` cluster is no longer first.
