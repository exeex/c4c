Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Retargeted the `using` import known-function path to build namespace-context-aware structured keys directly, and switched namespace value lookup to probe `known_fn_names` through that structured context key instead of reconstructing a rendered qualified string first.

## Suggested Next
Continue Step 3 by moving another `using` / alias / import lookup path off rendered-name bridge probes, with typedef or value-alias registration as the next narrow target.

## Watchouts
Anonymous namespace contexts can still fall back to the legacy rendered bridge when the current context chain does not already have an interned structured path, so follow-up slices should either normalize that path creation or keep the fallback isolated.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
