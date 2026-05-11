Status: Active
Source Idea Path: ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Local and Block Enum Static-Eval Paths

# Current Packet

## Just Finished

Plan Step 1 inventory for idea 167 is recorded. No implementation, test, plan,
idea, review, or proof-log files were edited.

Inventory and classifications:

- Parser block enum scope table:
  `src/frontend/parser/impl/statements.cpp:33` saves
  `parser.binding_state_.enum_consts` on block entry and restores it at
  `src/frontend/parser/impl/statements.cpp:85`.
  `src/frontend/parser/impl/types/struct.cpp:2999` copies that table for enum
  initializer evaluation, `src/frontend/parser/impl/types/struct.cpp:3035`
  adds earlier enumerators by `TextId`, and
  `src/frontend/parser/impl/types/struct.cpp:3062` publishes unscoped enum
  constants back to the parser binding state. Consumer:
  `src/frontend/parser/impl/support.cpp:323` / `:342` resolves enum
  initializer expressions by `unqualified_text_id`.
  Classification: scoped-`TextId` capable. It is parser-local enum initializer
  static evaluation, not the main HIR consteval route; the spelling branch at
  `support.cpp:335` is documented compatibility only and currently returns
  false.

- Sema scoped local/block enum static-eval stack:
  `src/frontend/sema/validate.cpp:1178`-`:1180` creates rendered, `TextId`,
  and structured scoped enum value stacks; `:1190`-`:1192` pops them.
  `bind_enum_constants_local` at `:1486` writes rendered values at `:1495`,
  `TextId` values at `:1499`, and local structured keys at `:1504`.
  `populate_const_eval_env` at `:1369`-`:1379` passes all scoped enum maps to
  `ConstEvalEnv`. Consumers include sema static assertions and foldable local
  const initializers through `evaluate_constant_expr`.
  Classification: scoped-`TextId` capable with a scoped local-key map. It is
  not a complete structured-domain carrier because `enum_variant_local_key`
  has only the enumerator `TextId`; the scope-stack position supplies the
  disambiguating domain.

- Shared `ConstEvalEnv` scoped enum lookup:
  `src/frontend/sema/consteval.hpp:152`-`:177` defines rendered enum maps as
  compatibility and scoped `TextId`/key maps as authority.
  `lookup_by_text` searches `enum_scopes_by_text` at `:467`-`:475`, and
  `lookup_by_key` searches `enum_scopes_by_key` at `:538`-`:547` before
  global enum maps.
  Classification: structured/scoped-`TextId` capable when callers provide the
  scoped maps; rendered-compatibility only when callers provide only
  `enum_consts`.

- HIR lowerer mutable enum mirror:
  `src/frontend/hir/hir_build.cpp:325` writes every collected enumerator into
  `Lowerer::enum_consts_`. Top-level collection calls
  `collect_enum_def(item, true)` at `hir_build.cpp:389`, which can also write
  structured global keys into `CompileTimeState`. Block/local lowering calls
  `collect_enum_def(n)` at `src/frontend/hir/impl/stmt/stmt.cpp:503`, which
  writes only rendered names. Block lowering saves/restores `enum_consts_` at
  `src/frontend/hir/impl/stmt/stmt.cpp:324` and `:335`.
  Classification: rendered-compatibility only for local/block enums; structured
  global-capable only for top-level enums.

- HIR lowerer consteval environment consumers:
  `src/frontend/hir/hir_types.cpp:315` passes `&enum_consts_` into
  `ConstEvalEnv`, and `:318` attaches only refreshed global structured enum
  maps. This feeds local/block-sensitive consumers such as constexpr-if
  lowering, ternary folding, switch case/range folding, local const binding
  folding, template value argument evaluation, and consteval call argument
  evaluation through `make_lowerer_consteval_env`.
  Classification: rendered-compatibility only for local/block enum constants.
  This is the main residual path from idea 167.

- HIR `eval_const_int_with_nttp_bindings` and static-member helpers:
  `src/frontend/hir/hir_types.cpp:504`-`:510` can use a structured enum key,
  then falls back to `enum_consts_` at `:514`-`:515`. Static-member initializer
  evaluation in `src/frontend/hir/impl/expr/scalar_control.cpp:535`-`:538`
  uses `static_eval_int` with rendered `enum_consts_` plus global structured
  maps. The direct enum-expression lowerer at `scalar_control.cpp:546`-`:548`
  still reads rendered `enum_consts_`.
  Classification: global/static-member structured-domain capable but unrelated
  to local/block conversion except as a compatibility boundary; the direct
  lowerer read is rendered-compatibility only for local/block enum values.

- `CompileTimeState` enum map:
  `src/frontend/hir/compile_time_engine.hpp:1773` stores rendered enum
  constants, and `:1778`-`:1785` stores structured mirrors only for
  `GlobalEnumConstant`. Engine env construction at
  `src/frontend/hir/impl/compile_time/engine.cpp:102` passes the rendered map
  plus global structured mirrors. Because block `collect_enum_def(n)` currently
  calls the rendered registration path, local/block enum constants can enter a
  global compile-time rendered mirror without scoped identity.
  Classification: rendered-compatibility only for local/block enums; global
  structured-domain capable for top-level enums.

## Suggested Next

First implementation target: HIR lowerer local/block enum handoff. Add scoped
local/block enum `TextId` and/or local-key maps beside `Lowerer::enum_consts_`
and preserve them with the same block lifetime as `enum_consts_`; populate
those maps from `collect_enum_def(n)` for non-global enum definitions; pass
them through `make_lowerer_consteval_env` so HIR constexpr-if, switch case,
ternary, local const, template value argument, and consteval-call argument
folding can use `ConstEvalEnv` scoped enum metadata. Keep the current rendered
`enum_consts_` lookup as the explicit no-metadata compatibility bridge, and
avoid registering local/block enums into `CompileTimeState` as unscoped
rendered globals unless the implementation also gives them scoped metadata.

Recommended focused proof command for that target:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|cpp_positive_sema_ok_enum_scope_local_over_global_c|positive_sema_ok_enum_scope_no_leak_after_block_c)$'`

## Watchouts

- Keep the route focused on local/block enum ownership and lifetime.
- Do not reopen the global/static-member conversion completed by idea 164
  unless only documenting a compatibility boundary.
- Treat rendered enum mirrors as compatibility only when structured or scoped
  `TextId` metadata is incomplete.
- A raw local enumerator `TextId` is not enough by itself for same-spelled
  block enums. The current sema route gets separation from the scope-stack
  position; the HIR route needs an equivalent scoped carrier or a stronger
  local-domain key.
- `CompileTimeState` currently has no local/block enum scope lifetime, so using
  it for block enums as a rendered global mirror is the riskiest leak/collision
  path.

## Proof

Inventory-only packet. Per delegation, no build/test command was run and
`test_after.log` was not written.
