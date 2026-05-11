Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Convert Remaining Metadata-Capable Paths
你該做code review了

# Current Packet

## Just Finished

Completed plan Step 5 reviewer-blocker repair for
`infer_call_result_type_from_callee` local-over-param function-pointer
shadowing.

Changed paths:
- `src/frontend/hir/hir_types.cpp`
- `tests/frontend/frontend_hir_lookup_tests.cpp`

`infer_call_result_type_from_callee` now checks active local TextId/LocalId
function-pointer signatures before parameter signatures, matching the existing
local-over-param value/type lookup authority. A complete local binding without
a function-pointer signature now stops inference instead of reopening a
same-spelled parameter fallback, while rendered compatibility remains fenced to
the existing rendered local/param compatibility sets and no-metadata path.

Focused `frontend_hir_lookup_tests` coverage now proves a same-spelled active
local function pointer shadows the parameter signature, and that a complete
local shadow without a function-pointer signature blocks parameter fallback.

## Suggested Next

Supervisor can hand the fixed slice to reviewer/supervisor acceptance, or choose
the next narrow Step 5 metadata-capable lookup path if more reviewer blockers
remain.

## Watchouts

- The fix intentionally treats a complete local TextId hit as authoritative
  even when no local function-pointer signature is present; that preserves
  shadowing and prevents a stale same-name parameter signature from leaking
  through.
- Existing param/local complete miss and no-metadata rendered compatibility
  tests still pass under the delegated subset.

## Proof

Ran delegated proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_struct_body_instantiation|llvm_gcc_c_torture_src_20000112_1_c|frontend_hir_tests|frontend_hir_lookup_tests|positive_sema_ok_local_fn_ptr_decl_c|backend_codegen_route_x86_64_function_pointer_param_direct_arg_observe_semantic_bir|cpp_positive_sema_c_style_cast_template_fn_ptr_param_type_parse_cpp|cpp_positive_sema_operator_this_out_of_class_runtime_cpp|cpp_positive_sema_template_deferred_method_cpp|cpp_positive_sema_template_struct_method_cpp|eastl_cpp_external_piecewise_construct_frontend_basic_cpp|eastl_cpp_external_utility_frontend_basic_cpp)$' > test_after.log`.

Result: passed; `test_after.log` contains 12/12 selected tests passing.
