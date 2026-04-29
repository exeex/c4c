Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Convert Covered Pure Text Lookup Maps To TextId

# Current Packet

## Just Finished

Step 2 completed for the string-pool direct-call rewrite path in
`src/backend/bir/lir_to_bir.cpp`.

`LoweredStringConstantMetadata` now owns a local `TextTable` for stripped
string-pool names, and `bytes_by_name` is keyed by `TextId` instead of raw
string spelling. `StringPointerAliasMap` still uses LIR SSA text for local
carrier lookup, but its authority for the referenced string-pool constant is
now the interned pool-name `TextId`. Direct global and alias lookups in
`rewrite_direct_call_string_pointer_args` resolve through the metadata text
table before consulting `bytes_by_name`.

Final `@pool` spelling is preserved by looking the accepted `TextId` back up in
the string metadata text table when rewriting the BIR call argument.
`bir::StringConstant::name` remains the retained display spelling, and no
`LinkNameId` was introduced for string constants.

## Suggested Next

Execute a Step 3 packet for the first `LinkNameId`-backed function, call, or
global path where semantic identity can replace raw string lookup authority
without changing final spelling or diagnostics.

## Watchouts

String-pool constants still intentionally do not carry `LinkNameId`; this
packet used only local `TextId` identity for pure text lookup. The direct-call
rewrite still bridges LIR calls to lowered BIR calls by function text and call
order, which belongs to a separate semantic-carrier route if the supervisor
wants to continue reducing compatibility bridges.

## Proof

Passed delegated Step 2 proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: build completed; `ctest` reported 100% tests passed, 0 failed out of
108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
