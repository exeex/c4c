Status: Active
Source Idea Path: ideas/open/101_hir_enum_const_int_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Dual-read semantic lookup paths

# Current Packet

## Just Finished

Step 2/3 implemented structured value-binding key/API scaffolding and dual-write registration mirrors for HIR global enum constants and global const-int bindings.

Completed work:
- Added `CompileTimeValueBindingKeyKind`, `CompileTimeValueBindingKey`, hash support, and key helpers in `compile_time_engine.hpp` for global enum constants and global const-int declarations.
- Added structured mirror storage/accessors beside `CompileTimeState::enum_consts_` and `const_int_bindings_`.
- Preserved legacy `register_enum_const(name, value)` and `register_const_int_binding(name, value)` APIs unchanged, and added overloads that always write the rendered-name map before recording the structured mirror.
- Updated `Lowerer::collect_enum_def` so only the top-level/global collection pass dual-writes enum constants when `NK_ENUM_DEF` has `namespace_context_id` plus `enum_name_text_ids[i]`; local/block enum lowering remains string-only.
- Updated `Lowerer::lower_global` to dual-write literal const-int and char const-int bindings when the declaration has complete source identity; incomplete metadata remains string-only.
- Left `sync_global_const_bindings` and all lookup behavior unchanged.

## Suggested Next

Next packet should implement Step 4 by adding structured-capable lookup helpers or overloads, then routing semantic value lookup through the structured mirror only at call sites that already have complete source identity. Preserve legacy rendered-name lookup as fallback, keep string-only call sites unchanged until metadata handoff exists, and observe structured/legacy mismatches without changing the selected semantic result. Keep `sync_global_const_bindings` string-only unless a real metadata bridge is added first.

## Watchouts

The new structured maps are mirrors only; no lookup behavior changed. `collect_enum_def` is shared by top-level collection and block-local statement lowering, so only explicit top-level callers should pass the structured-global registration flag. `Lowerer::lower_global` recomputes the structured key separately for int and char literal branches, which is intentionally small-scoped but can be consolidated later if that branch grows.

Do not remove legacy string maps, change constant-folding semantics, downgrade expectations, migrate idea 100 template/consteval definition registries, or expand into idea 102 struct/method/member/static-member identity work.

## Proof

Ran the supervisor-selected proof and preserved output in `test_after.log`:

```sh
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_value_arg_static_member_trait|cpp_hir_stmt_switch_helper)$') > test_after.log 2>&1
```

Result after follow-up local-enum guard: build succeeded; 5/5 selected tests passed.
