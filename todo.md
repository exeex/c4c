Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prefer Semantic Identity In Function, Call, And Global Paths

# Current Packet

## Just Finished

Step 3 completed for `FunctionSymbolSet` semantic membership in the
LIR-to-BIR function/global paths.

`FunctionSymbolSet` now stores `LinkNameId` membership separately from raw
symbol-name compatibility lookup. Known-function APIs prefer structured
`LinkNameId` membership when an id is available, and the remaining string-only
callers use the clearly named `is_known_raw_function_symbol` compatibility
boundary.

Scalar pointer global initializers now carry the structured
`LirGlobal::initializer_function_link_name_ids` id through `GlobalInfo` and use
that id before raw initializer spelling when resolving
`initializer_symbol_name_id` or known function-pointer addresses. Added focused
coverage proving a drifted raw initializer function spelling still resolves to
the covered function `LinkNameId` when the structured id is present, while the
raw spelling remains preserved as compatibility/final spelling.

## Suggested Next

Continue Step 3 on aggregate pointer-initializer function references or other
call/global paths only where a structured id carrier already reaches BIR. If no
per-reference id carrier exists, keep that boundary classified instead of
inventing local name matching.

## Watchouts

`GlobalAddress` and aggregate pointer-initializer offsets still carry only raw
parsed LIR symbol text, so `resolve_pointer_initializer_offsets` remains a
raw-string compatibility boundary. This packet intentionally did not add a
local offset-to-function-id shortcut because `LirGlobal::initializer_function_link_name_ids`
does not provide per-offset carriers.

## Proof

Passed delegated Step 3 proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: build completed; `ctest` reported 100% tests passed, 0 failed out of
108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
