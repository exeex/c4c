Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 2/3
Current Step Title: Define Retirement and Fence Policy; Retire or Narrow the First Complete Registry Family

# Current Packet

## Just Finished

Completed plan Step 2 / Step 3 for the first complete registry family:
`CompileTimeState` structured lookups now fail closed after complete structured
misses instead of reopening rendered registries. The fenced helpers cover
template functions, primary template structs, template-struct specializations,
and consteval functions. Rendered registry access remains available for
string-only and explicit no-metadata compatibility calls.

Added focused frontend HIR coverage proving complete structured misses reject
stale rendered names and no-metadata calls still use rendered compatibility.
Updated the existing lookup-focused coverage to match the fenced policy.

## Suggested Next

Pick the next rendered registry family from the Step 1 inventory and fence its
complete-structured lookup path without deleting display/order/payload storage.

## Watchouts

`is_complete_compile_time_registry_key` currently treats complete text identity
as `unqualified_text_id != kInvalidText` with no declaration pointer fallback;
calls with incomplete declaration metadata or `nullptr` remain rendered
compatibility boundaries. Do not extend this packet into lowerer
`template_global_defs_`, ctor/dtor maps, or overload maps; Step 1 identified
those as not having complete structured replacements yet.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the focused `frontend_hir_tests`
ctest pass.
