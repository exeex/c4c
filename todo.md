Status: Active
Source Idea Path: ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Proof and Closure Readiness

# Current Packet

## Just Finished

Plan Step 6 final proof passed for idea 167's local/block enum structured
mirror slice.

Converted local/block paths:

- HIR lowerer now carries scoped enum `TextId` and local-key maps beside the
  rendered `enum_consts_` compatibility mirror.
- Non-global `collect_enum_def(n)` populates those scoped maps from
  `enum_name_text_ids`.
- Block and statement-expression exits restore scoped map depth with
  `enum_consts_`, matching local/block enum lifetime.
- `make_lowerer_consteval_env` passes scoped enum metadata into `ConstEvalEnv`
  for constexpr-if, switch/range folding, ternary folding, local const folding,
  template value arguments, and consteval-call argument evaluation.
- Direct enum-expression lowering in `lower_var_expr` now uses
  `ConstEvalEnv::lookup(n)` so scoped TextId/key metadata is checked before
  rendered `enum_consts_`, and complete scoped misses fail closed through
  existing `ConstEvalEnv` behavior.

Coverage added:

- HIR constexpr-if coverage for same-spelled outer local and inner block enum
  constants verifies branch selection through lowerer static evaluation.
- HIR direct enum-expression coverage verifies `return Same;` lowers to the
  inner enum value while scoped and the restored outer value after block exit.
- Metadata-level scoped enum coverage sets scoped `Same` values to `17` and
  `29` while stale rendered `enum_consts_` says `Same = 404`; it proves scoped
  metadata wins and complete scoped misses do not recover through rendered
  spelling.
- Existing positive sema enum-scope cases continue to cover local-over-global
  and no-leak-after-block behavior.

Retained compatibility boundaries and removal conditions:

- Rendered `enum_consts_` remains only as a no-metadata compatibility mirror for
  enum references whose AST nodes lack complete TextId/key metadata. Removal
  condition: all enum-reference producers carry usable TextId/key metadata.
- Static-member initializer evaluation still uses its separate
  rendered/static-member bridge and global structured maps. Removal condition:
  that path receives a scoped local/block enum metadata carrier, or is split
  into a separate initiative if its lifetime requirements differ.
- `CompileTimeState` still has no local/block enum scope lifetime and should
  not be used as local/block enum authority. Removal condition: compile-time
  state gets an explicit scoped local/block enum lifetime model.

Closure readiness:

- No current blocker for closing this runbook slice after supervisor review.
- Suggested split if more work is desired: create a follow-up idea for
  static-member initializer / compile-time-engine local-block enum scoped
  lifetime, rather than expanding this completed lowerer-focused route.

## Suggested Next

Supervisor/plan-owner closure review for idea 167.

## Watchouts

- The local/block structured key remains the local enumerator key; scoped stack
  position supplies the local/block domain, matching `ConstEvalEnv` semantics.
- Keep static-member initializer / compile-time-engine scoped local enum work
  as a separate packet or idea if the supervisor wants to pursue that residual
  boundary.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_expr_scalar_control_helper|positive_sema_ok_enum_scope_local_over_global_c|positive_sema_ok_enum_scope_no_leak_after_block_c)$' > test_after.log`

Proof log: `test_after.log`.
