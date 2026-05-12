Status: Active
Source Idea Path: ideas/open/198_parser_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Template Instantiated Record Lookup

# Current Packet

## Just Finished

Step 4 hard-fenced
`eval_const_int_with_rendered_named_const_compatibility` in commit
`18b8a7757`.

The compatibility bridge now accepts an optional structured named-constant
`TextId` table. When that table is supplied and the node carries parser
named-constant metadata, structured lookup is authoritative: a hit returns the
`TextId` value, and a miss fails closed before rendered named-constant lookup.
The retained rendered lookup is explicitly documented as legacy/no-metadata
compatibility for callers such as HIR template probes that still pass rendered
NTTP names, with the removal condition tied to those callers carrying TextIds.

The direct parser-support residual metadata test proves that stale rendered
named constants cannot override complete parser metadata in the compatibility
bridge, including a qualified metadata miss, while preserving explicit
rendered no-metadata behavior.

Focused proof passed for
`cpp_hir_parser_support_residual_structured_metadata`. The accepted
full-suite baseline candidate for commit `18b8a7757` was identical green:
3137 passed, 0 failed, with the existing 12 disabled backend CLI tests.

## Suggested Next

Begin Step 5 by auditing template instantiated record lookup routes that can
derive candidate keys from rendered `template_origin_name` or adjacent
rendered spelling.

Bound the next executor packet to parser-owned template instantiated record
lookup in `src/frontend/parser/impl/types/base.cpp` and closely related parser
handoff tests. Classify each remaining rendered-origin lookup as one of:
metadata-rich route that must use structured record, owner, or domain metadata;
legacy/no-metadata compatibility that needs an owner/limitation/removal
comment; or out-of-scope HIR/Sema behavior that belongs outside this parser
plan.

For the first reachable metadata-rich route, replace rendered-origin authority
or fail closed after a complete structured miss, then add focused stale-rendered
template-origin proof if the route remains reachable.

## Watchouts

- Keep the work parser-owned; do not expand into Sema, HIR, LIR, BIR, or
  backend cleanup.
- Do not downgrade supported tests or replace semantic fixes with expectation
  rewrites.
- Do not treat parser diagnostics, AST display strings, source spelling, or
  final output text as semantic identity.
- Do not treat a raw `TextId` alone as complete semantic identity across
  scopes.
- Newly retained parser bridges need `legacy` or `deprecated` comments with
  owner, limitation, and removal condition.
- Step 5 owns template instantiated record lookup only. Do not drift back into
  record layout, owner recovery, named-const identity, or backend consteval
  work unless a separate lifecycle packet says so.
- Existing HIR/Sema template callers may still lack parser structured carriers.
  Preserve those as explicit legacy/no-metadata boundaries unless the packet
  proves parser-owned metadata is complete.
- Do not treat `template_origin_name` or rendered candidate keys as semantic
  authority when structured instantiated-record metadata is available.
- Step 3 reviewer watch item: retained no-metadata/template-instantiation
  fallbacks in `base.cpp` should be accounted for in Step 5 and the Step 6
  closure ledger.

## Proof

Step 4 focused proof passed:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_parser_support_residual_structured_metadata$"' > test_after.log 2>&1`

Proof log: `test_after.log`.

Baseline accepted:

`test_baseline.log` records commit `18b8a7757f56db06dab28fa91b198888d6efaef4`,
full-suite, 3137 passed, 0 failed, with the existing 12 disabled backend CLI
tests.
