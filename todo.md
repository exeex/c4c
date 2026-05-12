Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Fence Deduction, Collection, Seed, And Retry Paths

# Current Packet

## Just Finished

Step 3: Fence Deduction, Collection, Seed, And Retry Paths.

Status note: this packet fenced the deduction/build seed/retry portion of
Step 3, but the step is not closure-complete. Supervisor scan still found
rendered generated-call authority in `src/frontend/hir/impl/expr/call.cpp`
that must be fenced before Step 4 closure evidence.

Changed files:

- `src/frontend/hir/impl/templates/deduction.cpp`
- `src/frontend/hir/hir_build.cpp`
- `tests/frontend/frontend_hir_tests.cpp`

Completed work:

- Routed deduction result inference through `function_decl_nodes_` when a
  callee declaration carrier exists, using
  `CompileTimeState::find_template_def(primary_decl, rendered_name)` before
  deduction builds bindings.
- Added a local HIR-build template registry lookup boundary for collection,
  parameter-order recovery, seed recording, explicit/direct/deduced
  instantiation collection, consteval-template seed collection, and deferred
  retry.
- Made seed recording and deferred retry fail closed when a structured
  declaration carrier exists but misses the compile-time template registry.
- Kept rendered `find_template_def(name)` lookup only behind the no-declaration
  branch of that helper.
- Added focused HIR coverage proving template seed recording and deferred retry
  do not recover through stale rendered spelling after a complete structured
  declaration miss.

## Suggested Next

Continue Step 3 with a focused generated-call lowering fence in
`src/frontend/hir/impl/expr/call.cpp` before any Step 4 closure evidence.

The remaining rendered authority sites are:

- `ct_state_->has_template_def(n->left->name)`
- `ct_state_->find_template_def(n->left->name)` in the explicit
  template-call branch
- `ct_state_->find_template_def(n->left->name)` in the deduced-template
  branch

## Watchouts

- Retained compatibility: `src/frontend/hir/hir_build.cpp` still permits
  rendered `find_template_def(name)` only when `function_decl_nodes_` has no
  declaration carrier for the template name. Owner: HIR build collection/seed
  scaffolding. Limitation: no structured declaration identity is available.
  Removal condition: all generated template collection/seed callers carry a
  primary declaration identity instead of only rendered spelling.
- Retained compatibility: `src/frontend/hir/impl/compile_time/engine.cpp`
  still uses rendered compile-time lookup only when `TemplateCallInfo` has no
  `primary_template_decl`. Owner: compile-time replay of legacy/no-metadata HIR
  template calls. Limitation: old metadata cannot prove the primary
  declaration. Removal condition: generated HIR template calls require or
  synthesize `primary_template_decl`.
- Retained compatibility: `CompileTimeState::record_deferred_instance` still
  fills a missing `primary_def` from rendered spelling when the caller supplies
  no primary. Owner: compile-time registry bookkeeping. Limitation: only
  no-metadata callers use this branch after Step 2 replay precheck passes the
  resolved primary. Removal condition: deferred-instantiation APIs require the
  primary template declaration.
- Retained compatibility from Step 2: the builtin `"forward"` helper remains a
  rendered no-metadata bridge until that generated helper has a structured
  declaration carrier.
- Closure is not ready while `src/frontend/hir/impl/expr/call.cpp` can still
  use rendered generated-call lookup as semantic authority after structured
  identity is available.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains `frontend_hir_tests` passing 1/1.
