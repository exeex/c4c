Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Narrow Sema and Consteval Rendered Mirrors

# Current Packet

## Just Finished

Step 2 - Retire Parser Compatibility Overloads is complete enough to advance.
The parser bridge-retirement packets landed in:

| Commit | Result |
| --- | --- |
| `0e3a94b7e` | Retired the broad rendered typedef/type-compatibility parser-support overload family. Production callers now bind through structured `TextId` maps, and focused tests preserve stale-rendered-map closed-miss coverage. |
| `6aaed86f4` | Removed the broad rendered `eval_const_int` overload from ordinary resolution and fenced the retained no-metadata rendered named-constant path behind `eval_const_int_with_rendered_named_const_compatibility`. |
| `df4759b5f` | Removed the broad rendered `resolve_record_type_spec` ordinary boundary and fenced the retained parser-local rendered tag mirror behind `resolve_record_type_spec_with_parser_tag_map_compatibility`. |

Step 2 completion decision:

| Field | Result |
| --- | --- |
| Parser production structured misses | Covered parser-support typedef/type-compatibility, const-int, and record/tag families now fail closed for complete structured metadata misses or require an explicitly named compatibility API. |
| Retained parser bridges | Remaining rendered parser bridges are named as compatibility/no-metadata boundaries instead of ordinary overloads. |
| Focused proof | Each parser packet built its delegated targets and passed the supervisor-selected parser/support regression subset, with results recorded in `test_after.log` at the time of execution. |
| Baseline review | Supervisor compared old/new full-suite baselines after the hook reminder; both were 3135/3135 passing with the same disabled tests, then ran `scripts/plan_review_state.py accept-baseline`. |
| Lifecycle decision | Advance from Step 2 to Step 3: Narrow Sema and Consteval Rendered Mirrors. |

## Suggested Next

Begin Step 3 by re-reading the idea 167 sema/consteval bridge inventory and
selecting the first narrow rendered mirror family. Candidate starting points
from the runbook are rendered enum const maps, template/type/NTTP binding
mirrors, fallback canonical template names, and any source/link compatibility
`by_name` consteval bridge.

The next executor packet should name the exact sema or consteval mirror being
narrowed, inspect production callers before editing tests, and use focused
closed-miss proof for the touched metadata carrier.

## Watchouts

- Keep ABI, display, diagnostics, and final spelling output-only; Step 3 should
  not remove visible text just to reduce rendered-string grep count.
- Do not fold route-local generated-name cleanup into this plan; idea 169 owns
  route-local identity domains.
- `eval_const_int_with_rendered_named_const_compatibility` and
  `resolve_record_type_spec_with_parser_tag_map_compatibility` remain explicit
  compatibility boundaries from Step 2 and should not gain new ordinary
  production callers.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Lifecycle-only cleanup. No implementation or test files were changed.

Step 2 proof already landed with the three parser bridge commits:

- `0e3a94b7e`: built `frontend_parser_tests`,
  `frontend_parser_lookup_authority_tests`,
  `cpp_hir_parser_type_helper_residual_metadata_test`, and
  `cpp_hir_template_pattern_match_metadata_test`; ran 4/4 focused tests.
- `6aaed86f4`: built `frontend_parser_tests`,
  `frontend_parser_lookup_authority_tests`,
  `cpp_hir_parser_support_residual_metadata_test`, and
  `cpp_hir_template_pattern_match_metadata_test`; ran 4/4 focused tests.
- `df4759b5f`: built `frontend_parser_tests`,
  `frontend_parser_lookup_authority_tests`, and
  `cpp_hir_parser_support_residual_metadata_test`; ran 3/3 focused tests.

Baseline review was accepted after the supervisor compared old/new full-suite
baselines: both were 3135/3135 passing with the same disabled tests.
