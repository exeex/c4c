# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_legacy_string_lookup_resweep.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Demote Parser Lookup Fallbacks Where Structured Keys Exist

## Just Finished

Completed Step 2 first Sema-side structured-primary packet for consteval
function lookup.

Changed `src/frontend/sema/validate.cpp` and
`src/frontend/sema/consteval.cpp` so consteval function lookup checks the
structured key map before returning the rendered-name map result. Rendered-name
lookup remains the compatibility fallback when the structured mirror is absent,
and the structured/rendered pointer comparison remains advisory.

## Suggested Next

Next coherent packet: convert Sema consteval value/type binding lookup in
`src/frontend/sema/consteval.cpp` to structured-primary for
`ConstEvalEnv::lookup(const Node*)` and `resolve_type`, while preserving
rendered-name compatibility fallback for missing structured bindings.

Suggested proof command for that packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_sema_consteval|hir_consteval|negative_tests_bad_consteval|negative_tests_static_assert_consteval)' --output-on-failure`

## Watchouts

Do not edit HIR, LIR, BIR, or backend implementation for this frontend resweep
except to record a separate metadata handoff blocker under `ideas/open/`.
Rendered strings may remain for diagnostics, display, final spelling,
compatibility input, or explicit TextId-less fallback.

Parser already has many classified compatibility bridges from the prior cleanup
series. Avoid starting with broad parser typedef fallback removal; the Sema
consteval lookup packet is narrower and has direct structured mirrors already
available.

A strict "valid key miss returns null" version broke nested consteval calls
because some consteval body references still have valid structured keys without
matching structured function mirror entries. Treat those misses as compatibility
fallbacks unless the next packet repairs the missing mirror source.

## Proof

Proof passed:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_sema_consteval|hir_consteval|negative_tests_bad_consteval|negative_tests_static_assert_consteval)' --output-on-failure`

Proof log: `test_after.log`
