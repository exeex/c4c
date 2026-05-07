# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Argument Key Representation

## Just Finished

Completed the second Step 2 implementation slice at the parser key/helper
boundary:

- Replaced `TemplateInstantiationKey::Argument::Type`'s string mirror with a
  recursive `TypeComponent` payload derived from `TypeSpec` fields.
- Type components now cover base kind, enum underlying kind, qualified/source
  TextIds, template parameter owner/index metadata, parser record identity,
  qualifiers, declarator shape, arrays, vectors, template origins, deferred
  member typedef owner/member keys, and nested `TemplateArgRef` type/value
  cases.
- `make_template_instantiation_argument_key` now builds type payloads through
  `make_template_type_key_components(arg.type)` instead of
  `canonical_template_struct_type_key(arg.type)`.
- `TemplateInstantiationKeyHash` hashes the structured type component stream
  instead of a rendered type string.
- Direct expected-key tests now use the structured scalar type factory rather
  than old `"int"`/`"uint"` type-key strings.
- Added a focused parser test proving nested `TemplateArgRef::debug_text`
  differences do not affect the new type payload when the structured nested
  type payload matches.

## Suggested Next

Next coherent packet: audit and remove or retire parser-side direct callers of
`canonical_template_struct_type_key` that are now only display/debug or legacy
test coverage, then decide whether the helper should remain as a compatibility
API or be replaced by structured dump utilities.

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

## Proof

Supervisor escalated final validation to the broader structured parser subset.

Before command:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_.*structured_metadata)$') > test_before.log 2>&1`

After command for this slice:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_.*structured_metadata)$') > test_after.log 2>&1`

Result: PASS. Build succeeded and 23/23 selected tests passed. Proof log:
`test_after.log`.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: PASS. Before 23/23, after 23/23, no new failures.
