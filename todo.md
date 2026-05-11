Status: Active
Source Idea Path: ideas/open/168_static_member_compile_time_local_enum_lifetime.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Proof and Closure Readiness

# Current Packet

## Just Finished

Completed plan Step 6 final proof and closure-readiness check for idea 168.

Final inventory conclusion:
- No reachable static-member initializer or compile-time-engine enum lookup
  path currently has enough scoped local/block enum lifetime metadata to
  convert honestly.
- The lowerer-owned `ConstEvalEnv` scoped enum stacks remain the only
  scoped-lifetime capable local/block enum route observed during this idea.
- Static-member initializer evaluation currently sees global enum mirrors,
  NTTP bindings, structured static-member owner/member keys, and rendered
  compatibility maps, but not a scoped enum lifetime stack.
- `CompileTimeState` remains a global normalization registry. It must not be
  treated as local/block enum authority unless a future pending expression or
  work item carries an explicit scoped lifetime carrier.

Retained compatibility boundaries and removal conditions:
- `StaticEvalIntEnumLookupInput`: retained as global/static-eval compatibility.
  Removal/conversion condition: a caller needing local/block enum authority
  routes through `ConstEvalEnv` or adds an explicit scoped stack carrier.
- `CompileTimeState::enum_consts_` and `enum_consts_by_key_`: retained as
  rendered and global structured registries. Removal/conversion condition:
  engine work items carry explicit local/block enum lifetime metadata.
- `make_engine_consteval_env`: retained as global registry projection into
  `ConstEvalEnv`. Removal/conversion condition: engine-side pending work can
  prove scoped lifetime without copying lowerer block scopes past their source
  lifetime.
- `eval_struct_static_member_value_hir`: retained as a static-member/template
  compatibility bridge. Removal/conversion condition: callers pass honest
  enum scope lifetime metadata instead of empty or global-only maps.
- Static-member rendered maps `struct_static_member_decls_` and
  `struct_static_member_const_values_`: retained as compatibility mirrors;
  owner/member keyed maps are static-member authority only, not local/block
  enum authority. Removal/conversion condition: static-member initializer
  callers carry both owner/member identity and scoped enum lifetime metadata.

Residual blocker:
- None for closure readiness. The current result is a documented/fenced
  compatibility boundary, not a behavior conversion. A future conversion would
  require a new source initiative or plan packet that introduces a real scoped
  lifetime carrier for a reachable static-member or engine path.

## Suggested Next

Supervisor review for lifecycle closure or archival decision.

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
- Treat this idea as closure-ready for the documented boundary/fence outcome;
  do not claim a converted static-member scoped enum path.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`

`test_after.log` reports 5/5 tests passed. The build completed with no work to
do.
