Status: Active
Source Idea Path: ideas/open/126_bir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Classify Remaining BIR String Surfaces

# Current Packet

## Just Finished

Step 5 classified the remaining BIR string-keyed and raw-name surfaces after
the covered `TextId`, `LinkNameId`, and `BlockLabelId` conversions.

Added code comments at the retained surfaces that make their role explicit:
display/final spellings for values, params, functions, calls, inline asm, and
printer output; compatibility label spellings for raw-only BIR with structured
`BlockLabelId` preferred when present; compatibility type text and structured
type spelling bridges for aggregate layout; byte-offset keyed aggregate
pointer/leaf-slot maps; route-local SSA, slot, selector, phi, aggregate, and
provenance maps; `TextId`-keyed string byte payload lookup; raw function symbol
fallbacks and global/type maps as unresolved LIR-boundary compatibility
surfaces. No broad conversion or behavior change was made in this packet.

## Suggested Next

Proceed to Step 6 closure/reproof review: run the supervisor-selected broader
BIR cleanup proof, inspect the diff for testcase-overfit or silent raw-string
semantic authority, and decide whether the active source idea can close or
needs a follow-up split for upstream LIR/HIR metadata boundaries.

## Watchouts

Retained raw-only `FunctionSymbolSet`, aggregate pointer offsets, structured
type text, local SSA/slot/provenance maps, selector/dump/printer surfaces, and
global/type producer-boundary maps are now classified as compatibility,
display/dump/final spelling, byte-offset keyed layout payload, route-local
lowering handles, or unresolved LIR boundaries. This packet intentionally did
not implement new conversions or touch producer files.

## Proof

Passed delegated Step 5 proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: build completed; `ctest` reported 100% tests passed, 0 failed out of
108 run in the `^backend_` subset, with 12 disabled tests not run.
Proof log: `test_after.log`.
