# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Parser and Type-Helper Construction Sites

## Just Finished

Completed the Step 3 parser/type-helper cleanup slice:

- Removed the obsolete parser-side rendered template-struct key helper.
- Retired the direct rendered-key test in `frontend_parser_tests`; structured
  template argument identity coverage remains on the recursive type payload.
- Replaced the residual metadata direct-key assertion with structured
  `TypeComponent` coverage that checks tag `TextId` identity wins over stale
  rendered tag text.
- Verified no remaining production or focused-test references to the retired
  helper name remain.

## Suggested Next

Next coherent packet: advance the runbook past Step 3 by auditing the remaining
compatibility-text fallback notes in parser template-instantiation key payloads
and either narrow the fallback criteria or hand the lifecycle state to the plan
owner if Step 3 is exhausted.

## Watchouts

- Type payload compatibility text is now isolated as explicit
  `CompatibilityText`/`TypeCompatibilityText` fallback metadata. Removal
  criteria: all parser TypeSpecs that can enter template-instantiation keys
  must carry structured TextIds, qualified-name keys, template parameter
  metadata, record identity, or deferred-member keys for names and origins; no
  no-metadata legacy tag/origin spelling should be needed for key equality.
- The explicit legacy `$expr:` payload remains compatibility-only. Removal
  criteria: all parser NTTP carriers that can produce expression identity
  should provide either structured expression nodes, captured token payloads,
  or numeric values before lookup/dedup insertion.
- Keep expression payload shape metadata in equality and hash together; dropping
  either `depth` or `parent_index` can recreate flattened-preorder ambiguity.
- Do not weaken tests or mark supported cases unsupported as a shortcut around
  structured argument identity.
- Keep HIR/Sema/backend specialization keys out of this parser-key packet
  unless the supervisor delegates that boundary explicitly.
- The retired rendered-key helper was test-only at the start of this packet; no
  production display/debug caller needed to keep it.

## Proof

Supervisor-selected proof for this slice:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_.*structured_metadata)$') > test_after.log 2>&1`

Result: PASS. Build succeeded and 23/23 selected tests passed. Proof log:
`test_after.log`.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Before 23/23, after 23/23, no new failures.
