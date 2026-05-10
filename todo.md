Status: Active
Source Idea Path: ideas/open/159_sema_consteval_domain_table_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make value and local binding lookup structured-first

# Current Packet

## Just Finished

Completed Step 2, "Make value and local binding lookup structured-first."
`ConstEvalEnv::lookup(const Node*)` now keeps per-domain metadata miss state so
complete structured/TextId misses skip only the covered rendered enum, local,
named, or NTTP compatibility map. Empty but present metadata maps count as
complete lookup authority, and rendered `ConstMap`/interpreter-local `by_name`
paths remain available only for uncovered no-metadata compatibility domains.
Added focused sema lookup metadata tests for stale rendered consteval local and
named value fallbacks, including an unrelated no-metadata named fallback guard.

## Suggested Next

Start Step 3 from `plan.md`: make consteval function lookup structured-first,
including complete structured/TextId misses that do not reopen rendered
function-name fallback.

## Watchouts

- `lookup_rendered_compatibility` intentionally preserves no-metadata rendered
  fallback by domain; future steps should avoid collapsing per-domain miss
  state back into one global rendered-fallback gate.
- `ConstEvalValueLookupResult` now has enum/local/named/NTTP miss flags; any
  Step 4 NTTP changes should set `nttp_binding_metadata_miss` when extending
  lookup helpers outside `ConstEvalEnv`.
- `InterpreterBindings::by_name` is still exposed through `env.local_consts`
  beside `by_text` and `by_key`, but complete local metadata misses now skip
  the rendered local fallback.
- Do not weaken tests, mark supported paths unsupported, or rely on
  testcase-shaped shortcuts.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 34/34
passing `positive_sema_` tests. Also ran the focused pre-proof check
`cmake --build --preset default --target cpp_hir_sema_lookup_value_metadata_test && ctest --test-dir build --output-on-failure -R '^cpp_positive_sema_lookup_value_structured_metadata$'`,
which passed.
