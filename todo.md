Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Classify Remaining LIR String Surfaces

# Current Packet

## Just Finished

Completed Step 5 classification for retained LIR string reachability and
rendering surfaces touched by the current inventory. `signature_text` is now
documented as final LLVM header spelling plus template-comment compatibility
payload, with `collect_fn_refs` scanning explicitly classified as an unresolved
producer-carrier compatibility fallback because no structured signature
function-reference carrier exists locally.

Classified `init_text` as final LLVM initializer spelling whose string scanning
is only a legacy compatibility fallback behind
`initializer_function_link_name_ids`; classified reachability `names` and
`discardable_by_name` as compatibility fallback paths behind `LinkNameId`
reachability; classified signature printer string replacement as a compatibility
rendering bridge and verifier signature parsing as compatibility parity checks
against structured type mirrors.

## Suggested Next

Suggested next packet: execute Step 6 reproof/review for LIR cleanup and decide
whether the active runbook can close or whether the unresolved structured
signature-reference carrier should become a separate producer-boundary idea.

## Watchouts

No new producer carrier was invented in Step 5. `collect_fn_refs`
`signature_text` scanning remains intentionally retained as compatibility around
an unresolved producer boundary, not as accepted semantic authority. If future
work needs structured references inside function signatures or metadata, split
that as upstream metadata work instead of extending this classification packet.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the delegated proof output; CTest reported
100% pass for the enabled selected tests: 113 tests passed, 12 configured MIR
tests were disabled and not run.
