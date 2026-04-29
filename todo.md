Status: Active
Source Idea Path: ideas/open/125_lir_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Thread Semantic Identity Through Calls, Externs, And Globals

# Current Packet

## Just Finished

Completed Step 3 conversion for LIR global object reselection in
`src/codegen/lir/hir_to_lir/core.cpp` and
`src/codegen/lir/hir_to_lir/const_init_emitter.cpp`. `GlobalId` and
global `DeclRef` selection now preserve the existing duplicate preference
while matching by `LinkNameId` first, then `name_text_id`, and using rendered
raw names only when stable identities are absent. Constant initializer
global-name emission now uses the resolved `DeclRef` global object for
global references before considering function or legacy rendered-name paths.

Added focused coverage in `tests/frontend/frontend_hir_tests.cpp` for a
global pointer initializer whose semantic array global keeps a concrete
`GlobalId` while both the HIR global carrier name and initializer decl-ref raw
name are drifted into an existing rendered-name collision. Lowering must emit
`@semantic_global` and not redirect the initializer to `@rendered_shadow`.

## Suggested Next

Suggested next packet: convert the remaining Step 3 string-only reachability
surface in `eliminate_dead_internals` so global initializer and signature
references can seed internal-function retention from structured identities
where LIR already carries them.

## Watchouts

`select_global_object(const std::string&)` remains the compatibility path for
TextId-less globals and keeps the old rendered-name duplicate preference.
`eliminate_dead_internals` still treats global initializer text and signature
text as legacy string-only surfaces.

## Proof

Delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests$|frontend_lir_|backend_)'; } 2>&1 | tee test_after.log`

Passed. `test_after.log` contains the delegated proof output; CTest reported
100% pass for the enabled selected tests, with the configured disabled MIR
tests not run.
