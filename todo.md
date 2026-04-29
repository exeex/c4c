Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Classify Remaining LIR String Surfaces

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

Route review accepted Step 3 as complete for currently available structured
call, extern, global, and function-carrier metadata. The remaining
`collect_fn_refs` `signature_text` scan is retired from Step 3 executor scope
because no local structured producer carrier for signature metadata function
references is currently visible.

## Suggested Next

Suggested next packet: execute Step 5 classification for `collect_fn_refs`
signature-text scanning and any other retained LIR string surfaces. Treat the
signature scan as compatibility or an unresolved producer-carrier boundary
unless the executor finds an existing structured signature-reference carrier;
if such producer work is required, stop for lifecycle split instead of inventing
a local carrier.

## Watchouts

Global initializer reachability now has a structured function-id path; rendered
`init_text` scanning remains as compatibility fallback. Function signature text
is a legacy string-scanned surface to classify in Step 5, not a remaining Step
3 implementation packet.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the delegated proof output; CTest reported
100% pass for the enabled selected tests: 113 tests passed, 12 configured MIR
tests were disabled and not run.
