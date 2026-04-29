Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Thread Semantic Identity Through Calls, Externs, And Globals

# Current Packet

## Just Finished

Completed Step 3 conversion for the remaining LIR global-initializer
reachability seed in `eliminate_dead_internals`. `LirGlobal` now carries
structured function `LinkNameId`s referenced by its initializer, and
dead-internal elimination seeds from those ids before falling back to legacy
rendered `init_text` scanning.

Added focused frontend coverage for a global function-pointer initializer whose
semantic static helper keeps a structured initializer identity while the raw
initializer name is drifted into a rendered-name collision and the helper's raw
function carrier name is drifted. The semantic helper stays alive, while the
rendered-name collision is still eliminated.

## Suggested Next

Suggested next packet: review Step 3 for any remaining string-only signature
reachability surfaces that lack structured carriers, then either split a
follow-up carrier packet or hand Step 3 back for route review if no local
structured identity exists yet.

## Watchouts

Global initializer reachability now has a structured function-id path; rendered
`init_text` scanning remains as compatibility fallback. Function signature text
is still a legacy string-scanned surface because this packet did not introduce
new structured signature reference metadata.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the delegated proof output; CTest reported
100% pass for the enabled selected tests: 113 tests passed, 12 configured MIR
tests were disabled and not run.
