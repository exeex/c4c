# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Argument Key Representation

## Just Finished

Completed the first Step 2 implementation slice at the parser key/helper
boundary:

- `TemplateInstantiationKey::Argument` no longer exposes `std::string
  canonical_key` as the semantic field.
- Argument identity now routes through explicit payload variants:
  type, value expression, captured value tokens, legacy `$expr:` text, and
  numeric value.
- `TemplateInstantiationKeyHash` hashes the active structured payload variant
  instead of hashing one flattened argument string.
- `make_template_instantiation_argument_key` now constructs the active payload
  variant directly: expression nodes become ordered structured node metadata,
  including `depth` and `parent_index` so preorder child-role sequences cannot
  erase tree shape; captured tokens become `{kind, TextId}` entries, legacy
  rendered `$expr:` text remains isolated in the explicit compatibility
  variant, numeric NTTP values remain numeric, and type arguments are isolated
  in a `Type` payload.
- Direct tests that hand-filled or asserted `Argument::canonical_key` now build
  keys through the explicit factories or assert the value-expression payload
  kind without weakening expectations.
- Added a focused expression-shape regression covering two different NTTP trees
  that share the old flattened preorder role/kind sequence but differ by the
  parent of the final `Right` child.

## Suggested Next

Next coherent packet: replace the remaining type-side `Type` payload string
mirror with structured `TypeSpec`-derived payload components, especially the
nested `TemplateArgRef` cases that still flow through
`canonical_template_struct_type_key`.

## Watchouts

- The `Type` payload is still backed by `canonical_template_struct_type_key`;
  remove it only after type identity has structured components for base kind,
  qualifiers, pointers/references/arrays, record/typedef identity, and nested
  template arguments.
- The explicit legacy `$expr:` payload is compatibility-only. Removal criteria:
  all parser NTTP carriers that can produce expression identity should provide
  either structured expression nodes, captured token payloads, or numeric
  values before lookup/dedup insertion.
- Keep expression payload shape metadata in equality and hash together; dropping
  either `depth` or `parent_index` can recreate flattened-preorder ambiguity.
- Do not weaken tests or mark supported cases unsupported as a shortcut around
  structured argument identity.
- Keep HIR/Sema/backend specialization keys out of this parser-key packet
  unless the supervisor delegates that boundary explicitly.

## Proof

Supervisor escalated final validation to the broader structured parser subset.

Before command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_.*structured_metadata)$') > test_before.log 2>&1`

After restoring this slice, after command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_.*structured_metadata)$') > test_after.log 2>&1`

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Before 23/23, after 23/23, no new failures. Proof logs:
`test_before.log` and `test_after.log`.
