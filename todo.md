Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Fence Deduction, Collection, Seed, And Retry Paths

# Current Packet

## Just Finished

Step 3: Fence Deduction, Collection, Seed, And Retry Paths.

Status note: this packet fixed the `frontend_hir_lookup_tests` segfault
introduced during the structured template registry slices without weakening
the rendered-fallback fences. Step 3 remains open because rendered
generated-call authority in `src/frontend/hir/impl/expr/call.cpp` still needs
the focused fence recorded below before Step 4 closure evidence.

Changed files:

- `src/frontend/hir/impl/compile_time/engine.cpp`

Completed work:

- Copied `TemplateCallInfo` and the target callee name out of `module.expr_pool`
  before invoking deferred instantiation in `TemplateInstantiationStep`.
- Removed the dangling-reference path where deferred instantiation could append
  HIR expressions, reallocate `module.expr_pool`, and then reuse invalid
  template-call metadata while recording the deferred instance.
- Preserved the structured-primary precheck and complete-miss behavior; no
  rendered fallback fence was reopened.

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

- The regression was a metadata lifetime bug, not a lookup-policy failure:
  `TemplateInstantiationStep` must not hold references into `module.expr_pool`
  across `instantiate_fn(...)` because the callback can append expressions.
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

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains `frontend_hir_tests` and
`frontend_hir_lookup_tests` passing 2/2.
