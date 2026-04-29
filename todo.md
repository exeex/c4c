Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Thread Semantic Identity Through Calls, Externs, And Globals

# Current Packet

## Just Finished

Completed Step 3 conversion for `StmtEmitter` direct-call target lookup in
`src/codegen/lir/hir_to_lir/call/target.cpp`. When a call carries a valid
`LinkNameId`, local target resolution now treats that identity as authoritative
and does not fall through to legacy rendered-name function lookup after a
semantic miss. Unresolved external direct calls with a valid `LinkNameId` now
seed the LIR callee operand and extern-call declaration from the link-name
spelling instead of the raw decl-ref name.

Added focused coverage in `tests/frontend/frontend_hir_tests.cpp` for a call
whose raw decl-ref spelling collides with a defined local function while its
semantic `LinkNameId` names an external callee. Lowering must keep the
external `LinkNameId`, emit `@semantic_external_helper`, and produce an extern
declaration without letting the rendered local collision win.

## Suggested Next

Suggested next packet: convert the next remaining Step 3 global-init or extern
finalization surface where existing `LinkNameId` metadata can replace rendered
`@name` scanning or raw-name filtering while preserving string-only fallback
compatibility.

## Watchouts

`record_extern_call_decl` still has a raw-name local-function filter for
TextId-less compatibility paths; this packet avoids that path for valid
`LinkNameId` calls by resolving the extern name from the semantic table first.
`eliminate_dead_internals` still treats global initializer text and signature
text as legacy string-only surfaces.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the delegated proof output; CTest reported
100% pass for the enabled selected tests, with the configured disabled MIR
tests not run.
