Status: Active
Source Idea Path: ideas/open/167_local_block_enum_scope_static_eval_structured_mirrors.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Local/Block Collision Coverage

# Current Packet

## Just Finished

Plan Step 4 added focused HIR coverage for same-spelled local/block enum
constants statically evaluated through the converted lowerer path.
`frontend_hir_tests` now lowers a function with an outer local enum and an
inner block enum that both define `Same`, then verifies HIR constexpr-if
lowering keeps only the branches selected by the correct scoped values.

The test checks that the outer value is used before and after the inner block,
that the inner value is used inside the block, and that stale failure-return
literals from the wrong branches are absent from the lowered HIR. This observes
lowerer static-evaluation branch selection rather than sema diagnostics.

Step 2/3 implementation ledger:

- `Lowerer` carries scoped enum `TextId` and local-key maps beside the rendered
  `enum_consts_` compatibility mirror.
- Non-global `collect_enum_def(n)` populates those scoped maps from
  `enum_name_text_ids`, and `make_lowerer_consteval_env` passes them into
  `ConstEvalEnv`.
- Scoped maps are created only when declaration metadata exists, preserving the
  explicit no-metadata compatibility bridge for rendered-only callers.
- Block and statement-expression exits restore scoped map depth together with
  `enum_consts_`; local/block enum definitions are not registered into
  `CompileTimeState` as unscoped rendered globals.

Step 1 inventory ledger, preserved for route context:

- Parser block enum scope table: block entry saves and restores
  `parser.binding_state_.enum_consts`; enum initializer evaluation uses copied
  scoped tables and publishes unscoped constants back to parser binding state.
  Classification: scoped-`TextId` capable for parser-local enum initializer
  static evaluation; rendered spelling compatibility is not the main HIR
  consteval route.
- Sema scoped local/block enum static-eval stack: `validate.cpp` maintains
  rendered, `TextId`, and structured scoped enum value stacks and passes them
  to `ConstEvalEnv`. Classification: scoped-`TextId` capable with a scoped
  local-key map; stack position supplies the local/block domain.
- Shared `ConstEvalEnv` scoped enum lookup: scoped TextId/key maps are searched
  innermost to outermost before global maps, and complete metadata misses use
  the existing fail-closed behavior. Classification: structured/scoped-`TextId`
  capable when callers provide scoped maps; rendered compatibility only when
  callers provide no metadata.
- HIR lowerer mutable enum mirror before this packet: top-level
  `collect_enum_def(item, true)` could register structured globals, while
  block/local `collect_enum_def(n)` wrote only rendered `enum_consts_`.
  Classification before Step 2/3: rendered compatibility only for local/block
  enums.
- HIR lowerer consteval environment consumers: `make_lowerer_consteval_env`
  feeds constexpr-if, switch case/range folding, ternary folding, local const
  folding, template value arguments, and consteval-call argument evaluation.
  Classification before Step 2/3: rendered compatibility only for local/block
  enum constants; this was the selected first implementation target.
- HIR static-member and direct enum-expression helpers: global/static-member
  paths can use structured global maps, while direct rendered reads remain
  compatibility boundaries. Classification: related boundary, not converted by
  this local/block packet.
- `CompileTimeState` enum map: structured enum mirrors exist only for global
  enum constants and the flat rendered map has no local/block lifetime.
  Classification: rendered compatibility only for local/block enums; do not use
  it as local/block authority without scoped metadata.

## Suggested Next

Plan Step 5: convert the next metadata-capable local/block static-eval path, or
explicitly fence a remaining rendered-only boundary if the next path lacks
complete scope/domain metadata.

## Watchouts

- The local/block structured key is intentionally the local enumerator key;
  declaration-stack position supplies the scope domain, matching the existing
  `ConstEvalEnv` scoped lookup model.
- `enum_consts_` is still used by direct rendered lowerer compatibility paths;
  do not claim those are converted until their call sites pass metadata.
- `CompileTimeState` still has no local/block enum scope lifetime; keep
  local/block enum constants out of its flat rendered enum map unless a future
  packet adds scoped metadata there too.
- Nearby same-feature cases examined: HIR constexpr-if exercises
  `make_lowerer_consteval_env` and scoped enum lookup directly; direct enum
  expression lowering still uses the rendered compatibility mirror and is out
  of reach for this coverage packet until that call site is converted.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_expr_scalar_control_helper|positive_sema_ok_enum_scope_local_over_global_c|positive_sema_ok_enum_scope_no_leak_after_block_c)$' > test_after.log`

Proof log: `test_after.log`.
