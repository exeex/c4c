Status: Active
Source Idea Path: ideas/open/159_sema_consteval_domain_table_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Make consteval function lookup structured-first

# Current Packet

## Just Finished

Completed Step 3, "Make consteval function lookup structured-first."
`lookup_consteval_function` now checks structured function metadata first, uses
TextId metadata when no structured map is provided, treats complete structured
or TextId misses as authoritative, and falls back to rendered `consteval_fns`
only when the callee carries no usable function metadata. Added focused
consteval function tests for stale rendered fallback rejection, TextId-only
misses, and no-metadata rendered compatibility. Follow-up review gap fixed:
`CompileTimeState` now exposes consteval function TextId/structured maps and
HIR compile-time engine consteval call sites pass them into
`evaluate_consteval_call`, so real pending/nested HIR consteval reduction does
not fail closed after metadata-rich nested calls.

## Suggested Next

Start Step 4 from `plan.md`: make template type and NTTP binding lookup
structured-first, including complete metadata misses that do not reopen
rendered binding fallbacks.

## Watchouts

- HIR compile-time `evaluate_consteval_call` sites now pass function
  TextId/structured maps from `CompileTimeState`; future new call sites should
  do the same for metadata-rich nested calls.
- A local exploratory run of `frontend_parser_lookup_authority_tests` still
  fails an unrelated value-domain assertion:
  `consteval value lookup should reject rendered NTTP fallback after a qualified
  structured value-domain key misses`.
- Do not weaken tests, mark supported paths unsupported, or rely on
  testcase-shaped shortcuts.

## Proof

Ran delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and 34/34
passing `positive_sema_` tests. Also ran the focused pre-proof check
`cmake --build --preset default --target frontend_parser_tests && ctest --test-dir build --output-on-failure -R '^frontend_parser_tests$'`,
which passed. Follow-up focused HIR proof also passed:
`cmake --build --preset default --target frontend_hir_lookup_tests && ctest --test-dir build --output-on-failure -R '^frontend_hir_lookup_tests$'`.
