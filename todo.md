Status: Active
Source Idea Path: ideas/open/168_static_member_compile_time_local_enum_lifetime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Static-Member and Compile-Time Enum Paths

# Current Packet

## Just Finished

Step 1 inventory for static-member initializer and compile-time enum lookup
paths.

Static-member initializer evaluation call sites:
- `src/frontend/hir/hir_types.cpp`:
  `Lowerer::layout_struct` evaluates `static constexpr` member initializers
  with `static_eval_int` or `eval_const_int_with_nttp_bindings`, seeded from
  `refresh_global_consteval_structured_maps`. Classification:
  rendered/static-member compatibility only for the static-member path; it has
  global enum key metadata but no local/block scope stack, so it is not scoped
  lifetime capable for idea 168.
- `src/frontend/hir/impl/templates/struct_instantiation.cpp`:
  `Lowerer::record_instantiated_template_struct_field_metadata` evaluates
  instantiated static-member initializers with NTTP bindings and global enum
  keys. Classification: rendered/static-member compatibility only; no
  local/block enum lifetime carrier reaches this helper.
- `src/frontend/hir/impl/expr/scalar_control.cpp`: static-member scalar
  lowering re-evaluates a found declaration initializer through
  `static_eval_int` with rendered enum constants plus global enum keys.
  Classification: rendered/static-member compatibility only; it can consume
  structured static-member owner/member keys, but not local/block enum scope
  metadata.
- `src/frontend/hir/impl/templates/templates.cpp`:
  `eval_struct_static_member_value_hir` evaluates static member declarations
  with `eval_const_int` and optional NTTP bindings, using empty enum/global
  maps. Classification: rendered/static-member compatibility only for
  template/static-member bridging; local/block enum lookup is not represented.
- `src/frontend/hir/impl/templates/value_args.cpp` and
  `src/frontend/hir/impl/templates/deferred_nttp.cpp` call
  `eval_struct_static_member_value_hir` from template value-argument,
  instantiated static-member, and deferred NTTP paths. Classification:
  rendered/static-member compatibility only; the caller can provide template
  owner/NTTP information but not local/block enum lifetime metadata.

`CompileTimeState` enum/constant lookup registries:
- `src/frontend/hir/compile_time_engine.hpp`: `CompileTimeState::enum_consts_`
  is the rendered compatibility map, and `enum_consts_by_key_` is a structured
  global enum mirror registered only for
  `CompileTimeValueBindingKeyKind::GlobalEnumConstant`. Classification:
  scoped-lifetime incapable for local/block enums; it is global-domain capable
  only.
- `src/frontend/hir/hir_build.cpp`: `Lowerer::collect_enum_def` sends global
  enum constants into `CompileTimeState`, while local/block enum constants go
  into `enum_const_scopes_by_text_` and `enum_const_scopes_by_key_` on the
  lowerer. Classification: scoped-lifetime capable only while evaluation uses
  `Lowerer::make_lowerer_consteval_env`; intentionally not copied into
  `CompileTimeState`.
- `src/frontend/hir/impl/compile_time/engine.cpp`:
  `make_engine_consteval_env` copies only `CompileTimeState` global enum and
  const-int key maps into `ConstEvalEnv`; it does not carry enum scope stacks.
  Classification: unreachable by well-formed local/block enum lifetimes during
  engine normalization unless a future work item records explicit scoped
  lifetime metadata in the pending expression/work item.
- `src/frontend/sema/consteval.hpp`: `ConstEvalEnv` already has
  `enum_scopes_by_text` and `enum_scopes_by_key`, and lookup searches scoped
  maps innermost-first before global maps. Classification: scoped-lifetime
  capable when a lowerer-owned env supplies those stacks.
- `src/frontend/sema/type_utils.cpp`: `static_eval_int` accepts
  `StaticEvalIntEnumLookupInput` with rendered, TextId, and structured maps,
  but no scoped stack. Classification: global structured capable and
  rendered-compatibility only; not local/block scoped-lifetime capable as
  currently shaped.
- `src/frontend/hir/hir_types.cpp`:
  `Lowerer::eval_const_int_with_nttp_bindings` can use global enum keys and
  NTTP text bindings, then rendered enum fallback. Classification:
  rendered/static-member compatibility only for static-member initializers; not
  scoped-lifetime capable because it has no enum scope stack input.

Rendered/static-member compatibility bridges:
- Rendered enum maps retained in `StaticEvalIntEnumLookupInput` and
  `ConstEvalEnv::enum_consts` are no-metadata compatibility bridges.
- Static-member maps `struct_static_member_decls_` and
  `struct_static_member_const_values_` remain rendered compatibility mirrors;
  owner/member keyed maps `*_by_owner_` are structured static-member authority
  but do not imply local/block enum lifetime authority.
- `eval_struct_static_member_value_hir` is the main bridge that can hide
  enum-lifetime gaps because it evaluates static-member initializers without
  lowerer enum scope stacks.

Tests that may exercise this boundary:
- `frontend_hir_tests`: local/block enum scope coverage exists in
  `test_hir_local_block_enum_consteval_scopes_keep_same_spelled_values_distinct`;
  classification: scoped-lifetime capable lowerer env path, not static-member
  initializer.
- `cpp_hir_sema_consteval_type_utils_structured_metadata`: covers
  `static_eval_int`, same-spelled enum domains, and `ConstEvalEnv` scoped enum
  metadata; classification: direct unit coverage for utility/env behavior.
- `frontend_hir_lookup_tests`: covers static-member NTTP/static-member scalar
  lookup and structured owner/member bridges; classification:
  rendered/static-member compatibility bridge coverage, not local/block enum
  lifetime coverage.
- `frontend_parser_lookup_authority_tests`,
  `frontend_hir_tests`, and
  `cpp_hir_static_member_base_structured_metadata`: cover static-member owner,
  base, and stale-rendered rejection behavior; classification:
  static-member compatibility/owner-key coverage.
- `cpp_hir_template_value_arg_static_member_trait`: exercises a template
  static-member value-arg path; classification: static-member/template bridge
  coverage.

First target:
- Documentation/fence target first: record in code comments or a small helper
  boundary around `eval_struct_static_member_value_hir` and the two
  static-member initializer `static_eval_int` call sites that these paths are
  global/static-member compatibility only and must not claim local/block enum
  authority until an explicit enum scope stack or equivalent lifetime carrier
  is passed.
- First implementation target only if the next step elects conversion:
  extend `StaticEvalIntEnumLookupInput` or route static-member initializer
  evaluation through `ConstEvalEnv` only for call sites that can honestly pass
  `enum_const_scopes_by_text_` and `enum_const_scopes_by_key_`. Current
  inventory found no static-member initializer caller that already carries that
  lifetime metadata.

## Suggested Next

Execute Step 2 from `plan.md`: define the scoped lifetime model and decide
whether the next packet is a documentation fence for
`eval_struct_static_member_value_hir`/static-member initializer helpers or a
small carrier change that routes an existing lowerer-owned enum scope stack
into a genuinely reachable evaluation path.

## Watchouts

- Do not edit implementation or tests before recording the Step 1 inventory.
- Do not reopen idea 167 lowerer scoped maps to force a static-member case
  through the completed route.
- Do not treat `CompileTimeState`, raw `TextId`, rendered spelling, or a
  static-member bridge as local/block enum authority without explicit scoped
  lifetime metadata.
- The engine-owned `CompileTimeState` maps are global accumulators; copying
  local/block enum scope stacks into them would extend lifetime past block
  scope and would be route drift unless paired with an explicit pending-item
  lifetime model.
- Static data member initializers appear unreachable by well-formed
  local/block enum lifetimes in the current frontend shape; avoid fabricating a
  collision test until a real scoped-lifetime caller is identified.

## Proof

Not run; inventory-only packet and supervisor explicitly requested no
build/test command and no `test_after.log`.

Recommended focused proof for the next code or documentation slice:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$'`
