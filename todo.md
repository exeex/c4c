Status: Active
Source Idea Path: ideas/open/100_hir_compile_time_template_consteval_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read lookup paths

# Current Packet

## Just Finished

Step 4 implementation packet completed in `src/frontend/hir/impl/templates/templates.cpp`: routed `Lowerer::find_template_struct_specializations(const Node* primary_tpl)` through `ct_state_->find_template_struct_specializations(primary_tpl, primary_tpl->name)` before consulting the lowering-owned rendered-name fallback map.

## Suggested Next

Next coherent packet: review the remaining Step 4 declaration-aware lookup wrappers and route the next one with an existing structured helper plus its rendered-name fallback.

## Watchouts

This packet intentionally changed only the Lowerer wrapper. The local `template_struct_specializations_` fallback remains in place and is consulted only when the structured `CompileTimeState` lookup has no hit.

## Proof

Proof succeeded and was written to `test_after.log`.

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_lookup_tests|cpp_positive_sema_template_constexpr_member_runtime_cpp|cpp_positive_sema_if_constexpr_template_chain_cpp)$') > test_after.log 2>&1
```

Result: build completed and all 3 selected tests passed.
