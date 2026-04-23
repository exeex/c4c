Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Retargeted namespace-qualified typedef lookup and `using` type-alias registration to store and probe structured `QualifiedNameKey` entries keyed by namespace context, so the parser can resolve imported aliases and typedefs without relying on rendered `A::B` strings as the primary semantic key.

## Suggested Next
Continue Step 3 by moving the remaining `using` value-alias import path off rendered-name bridge probes, especially the `using_value_aliases` registration and namespace value lookup fallback that still carry compatibility strings as their authoritative payload.

## Watchouts
Anonymous namespace contexts still fall back to the legacy rendered bridge when lookup cannot find an interned namespace path, and `using` value aliases still store rendered names directly, so the next slice should keep string fallback isolated rather than expanding it.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
