Status: Active
Source Idea Path: ideas/open/84_parser_qualified_name_structured_lookup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget `using` / alias / import lookup paths to structured identity

# Current Packet

## Just Finished
Retargeted the remaining `using` value-alias import path to store structured alias targets, probe imported function names through interned `QualifiedNameKey` identity, and render bridge text lazily during lookup instead of keeping rendered qualified names as the authoritative alias payload.

## Suggested Next
Continue Step 3 by isolating the last string-keyed namespace value-variable fallback from the structured function path, especially the anonymous-namespace and `has_var_type(...)` compatibility cases that still require rendered bridge names.

## Watchouts
Anonymous namespace contexts and value-variable tables still need compatibility strings on the fallback path, so the next slice should keep bridge rendering narrowly contained instead of widening into a repo-wide var-identity migration.

## Proof
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'`
Passed. Proof log: `test_after.log`
