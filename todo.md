# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/base.cpp` preliminary template-argument
evaluation cluster that started at the deletion-probe residual near line 3858.
The preliminary type-binding helper now records template-parameter `TextId`
metadata beside the existing rendered binding names, resolves `tag_text_id` or
`template_param_text_id` before the explicit no-metadata rendered fallback, and
routes functional-cast and `sizeof(type)` checks through the same structured
path. The packet added
`cpp_hir_parser_type_base_prelim_eval_structured_metadata`, which proves a
stale rendered template-parameter name does not block functional-cast NTTP
preliminary evaluation from materializing the structured `Box<unsigned, 7>`
instantiation key when the structured parameter `TextId` matches.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/base.cpp:4484`.

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
- The deletion probe log for this packet is
  `/tmp/c4c_typespec_tag_deletion_probe_step4_base_prelim.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 106 of 106
delegated tests, including the new
`cpp_hir_parser_type_base_prelim_eval_structured_metadata` test, the existing
parser type-base structured metadata tests, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=105 failed=0 total=105`
before and `passed=106 failed=0 total=106` after. There are no new failing
tests; the pass-count increase over the current baseline is the new parser
type-base preliminary-evaluation structured-metadata test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_base_prelim.log`. The first emitted
`base.cpp` residual is now
`src/frontend/parser/impl/types/base.cpp:4484`, outside this packet's owned
local preliminary-evaluation cluster, so the targeted residual around the prior
`base.cpp:3858` cluster is no longer first.
