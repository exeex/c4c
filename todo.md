Status: Active
Source Idea Path: ideas/open/101_hir_enum_const_int_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory enum and const-int value-binding surfaces

# Current Packet

## Just Finished

Step 1 inventory completed for enum constant and const-int value-binding surfaces.

Main registration surfaces:
- HIR enum constants are registered string-only in `Lowerer::collect_enum_def`: `enum_consts_[name] = value` plus `CompileTimeState::register_enum_const(name, value)`. The source `NK_ENUM_DEF` has enough structured identity for a mirror now: `namespace_context_id`, `enum_name_text_ids[i]`, and the rendered spelling.
- HIR global const-int bindings are registered string-only in `Lowerer::lower_global` and `sync_global_const_bindings`: `const_int_bindings_[g.name] = value` plus `CompileTimeState::register_const_int_binding(g.name, value)`. `lower_global` has the declaration node and can derive structured global name identity now; `sync_global_const_bindings` only has `HirGlobal::name`, so it remains legacy/fallback unless metadata is handed through.
- Local const bindings live in `FunctionCtx::local_const_bindings`; sema already has structured local const mirrors, but HIR lowering currently passes only the string map to `ConstEvalEnv`.
- Template NTTP bindings are separate from this migration. They already have structured consteval mirrors in `bind_consteval_call_env`; do not migrate idea 100 template/consteval definition registries here.

Main lookup and handoff surfaces:
- `ConstEvalEnv::lookup(const Node*)` already has text/key lookup helpers, but returns the legacy string result after advisory parity checks. HIR-created envs generally populate only `enum_consts`, `named_consts`, and `local_consts`, so structured lookup is currently sema-capable but HIR fallback-only.
- HIR constant-expression env construction is string-only in `lower_global_init`, `lower_if_stmt`, `lower_local_decl_stmt`, switch/case lowering, ternary folding, consteval call lowering, pending consteval evaluation, static assert evaluation, and compile-time engine consteval call helpers.
- Direct HIR enum lookups in `eval_const_int_with_nttp_bindings`, `lower_var_expr` scalar constant folding, static member constexpr folding, and template struct field metadata are string-only because they call `enum_consts_.find(n->name)` or `static_eval_int(..., enum_consts_)` without structured env metadata.
- `CompileTimeState::enum_consts()` and `const_int_bindings()` expose only rendered-name maps to the engine. `CompileTimeState::dump` uses counts for diagnostics/debug visibility and should remain rendered-name-compatible.

Structured-capable now:
- Sema global enum and const-int maps already dual-write text/key mirrors and populate `ConstEvalEnv` with `*_by_text` and `*_by_key`.
- HIR enum registration from `NK_ENUM_DEF` can dual-write a structured global enum key when `namespace_context_id` and `enum_name_text_ids[i]` are present.
- HIR global const-int registration from the original declaration node can dual-write a structured global const key when `namespace_context_id` and `unqualified_text_id` are present.

String-only or bridge-required now:
- Engine resync from `HirGlobal` is rendered-name-only until `HirGlobal` carries source identity or the engine can use an existing declaration/metadata bridge.
- HIR local const bindings, local enum block save/restore, and switch/case env construction remain string-only until `FunctionCtx` owns parallel text/key maps.
- Member/static-member scoped constants under `struct_static_member_const_values_` remain out of the first packet; owner identity belongs to idea 102 and should stay on legacy fallback here.
- Template/static member constexpr paths that call `eval_const_int_with_nttp_bindings` are not ready for structured enum lookup until the expression evaluator receives source-key maps.

First implementation packet boundary: Step 2/3 key/API scaffolding for HIR global enum constants and global const-int bindings only. Add HIR-owned value-binding key types and structured mirror storage/accessors beside `CompileTimeState::enum_consts_` and `const_int_bindings_`, add overloads that accept structured key plus rendered fallback spelling, and dual-write from `Lowerer::collect_enum_def` and `Lowerer::lower_global`. Preserve existing string-only registration APIs for `sync_global_const_bindings` and any incomplete metadata path.

## Suggested Next

Execute the first code-changing Step 2/3 packet: define structured HIR value-binding keys and dual-write only the structured-capable global enum and global const-int registration paths while preserving rendered-name maps and all existing lookup behavior.

## Watchouts

Fallback policy: structured lookup is an advisory mirror until a complete key is available; registration must always write the legacy rendered-name map; lookup must prefer the current legacy result unless a later packet explicitly changes a structured-capable call site with legacy fallback and parity observation.

Do not remove legacy string maps, change constant-folding semantics, downgrade expectations, migrate idea 100 template/consteval definition registries, or expand into idea 102 struct/method/member/static-member identity work. Keep `sync_global_const_bindings` string-only unless the next packet also adds a real metadata bridge.

## Proof

Inventory-only packet; no build/test proof required and `test_after.log` was not rewritten.

Recommended narrow proof command for the next code-changing packet:

```sh
cmake --build build --target c4cll && ctest --test-dir build -R '^(cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_value_arg_static_member_trait|cpp_hir_stmt_switch_helper)$' --output-on-failure | tee test_after.log
```
