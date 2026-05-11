Status: Active
Source Idea Path: ideas/open/168_static_member_compile_time_local_enum_lifetime.md
Source Plan Path: plan.md
Current Step ID: 2/5
Current Step Title: Define the Scoped Lifetime Model for Convertible Paths and Fence Remaining Candidate Paths

# Current Packet

## Just Finished

Completed plan Step 2/5 by fencing the static-member and compile-time-engine
enum lookup boundaries as compatibility/global-only, with no behavior changes.

Fenced boundaries:
- `StaticEvalIntEnumLookupInput` now states that its rendered/TextId/key maps
  do not carry local/block enum scope lifetime and that scoped authority must
  use an explicit carrier such as `ConstEvalEnv`.
- `CompileTimeState` constant registries now state they are global
  normalization state, not local/block enum authority, and must not receive
  lowerer scope stacks without a pending-item lifetime model.
- `make_engine_consteval_env` now states that engine normalization consumes
  global `CompileTimeState` registries only; copying lowerer enum scopes there
  would outlive the source block.
- `eval_struct_static_member_value_hir` now states that static-member
  initializer probing is a template/static-member compatibility bridge and has
  no local/block enum lifetime stack.
- The static-member initializer re-evaluation paths in
  `Lowerer::lower_struct_def`,
  `Lowerer::record_instantiated_template_struct_field_metadata`, and
  `Lowerer::lower_var_expr` now state that they use global enum mirrors and
  rendered/static-member compatibility only.

Non-converted classifications and removal conditions:
- `CompileTimeState::enum_consts_` and `enum_consts_by_key_`: retained as
  rendered and global structured registries. Removal/conversion condition:
  a pending expression/work item carries explicit local/block scope lifetime
  metadata.
- `StaticEvalIntEnumLookupInput`: retained as global/static-eval compatibility.
  Removal/conversion condition: callers needing scoped lifetime move to
  `ConstEvalEnv` or add a scoped stack carrier.
- `eval_struct_static_member_value_hir`: retained as static-member/template
  compatibility. Removal/conversion condition: callers pass honest enum scope
  lifetime metadata instead of empty or global-only maps.
- Static-member `struct_static_member_*` rendered maps remain compatibility
  mirrors; owner/member keyed maps are static-member authority only, not
  local/block enum authority.

## Suggested Next

Execute a focused Step 5 follow-up if the supervisor wants more fencing:
search for any remaining static-member initializer callers that bypass the
newly documented boundaries and either classify them in comments or confirm
they route through the fenced helpers.

## Watchouts

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
- This packet was comment-only. A future behavior change still needs a concrete
  scoped lifetime model before converting any path.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`

`test_after.log` reports 5/5 tests passed.
