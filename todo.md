Status: Active
Source Idea: ideas/open/16_ref_overload_method_link_name_binding.md
Source Plan: plan.md

# Current Packet

## Just Finished

- Activated a narrow plan for the ref-overload method direct-call binding
  regression.
- Isolated the first packet to member-call direct-call metadata in
  `src/frontend/hir/hir_expr_call.cpp`.
- Patched resolved member-call lowering to bind the direct-call `DeclRef` from
  the concrete selected overload name.
- Rebuilt the compiler and proved the focused six-test ref-qualified runtime
  subset passes.

## Suggested Next

- Commit this narrow ref-overload binding fix as its own slice.
- Keep the remaining two failures separate:
  `cpp_positive_sema_c_style_cast_template_fn_ptr_rvalue_ref_return_type_parse_cpp`
  and `cpp_positive_sema_template_struct_specialization_runtime_cpp`.

## Watchouts

- Do not weaken tests or merge `&` / `&&` overload identities.
- Keep parser and template-specialization failures out of this packet.
- The worktree already has unrelated user edits in `AGENTS.md`; leave them
  untouched.

## Proof

- Passed: `cmake --build build -j`
- Passed: `ctest --test-dir build --output-on-failure -R 'cpp_positive_sema_(c_style_cast_base_ref_qualified_method|c_style_cast_ref_qualified_method|forwarding_ref_qualified_member_dispatch|function_return_temporary_member_call_runtime|ref_overload_method_basic|ref_overload_method_reads_arg)_cpp'`
