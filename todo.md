Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Thread Semantic Identity Through Calls, Externs, And Globals

# Current Packet

## Just Finished

Completed Step 3 conversion for `eliminate_dead_internals` in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`. Dead-internal reachability now
tracks structured `LinkNameId` references beside legacy rendered-name
references, uses `LirFunction::link_name_id` to identify discardable functions
when present, and uses `LirCallOp::direct_callee_link_name_id` for direct-call
reachability. Rendered `@name` scanning remains for signature text, global
initializer text, indirect/string-only calls, call-argument function references,
and other legacy string operands.

Added focused coverage in `tests/frontend/frontend_hir_tests.cpp` for a static
helper whose raw HIR function name is corrupted while its semantic
`LinkNameId` remains valid; lowering must keep the discardable helper reachable
through the direct-call `LinkNameId`.

## Suggested Next

Suggested next packet: move the next remaining semantic lookup surface from
Step 3, likely extern/global-init or StmtEmitter call lookup convergence,
through the same structured-first pattern while preserving string-only
fallbacks.

## Watchouts

`eliminate_dead_internals` still treats global initializer text and signature
text as legacy string-only surfaces, so future structured global-init metadata
would need a separate packet. The direct-call callee string is no longer used
as authority when `direct_callee_link_name_id` is present, but direct-call
argument text is still scanned for function references passed as values.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Supervisor acceptance proof widened to include the touched HIR test binary:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the supervisor acceptance proof output; CTest
reported 100% pass for the enabled selected tests, with the configured
disabled MIR tests not run.
