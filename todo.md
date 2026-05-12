Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Fence Generated Call And Replay Lookup

# Current Packet

## Just Finished

Step 2: Fence Generated Call And Replay Lookup.

Changed files:

- `src/frontend/hir/hir_ir.hpp`
- `src/frontend/hir/impl/expr/call.cpp`
- `src/frontend/hir/impl/compile_time/engine.cpp`
- `src/frontend/hir/compile_time_engine.hpp`
- `tests/frontend/frontend_hir_tests.cpp`

Completed work:

- Extended `TemplateCallInfo` with `primary_template_decl` so generated
  template-call metadata can preserve the already-known primary template
  declaration identity.
- Populated that identity from the `tpl_fn` recovered in generated call
  lowering and the builtin forwarding-helper path.
- Updated deferred template replay to resolve the primary through
  `CompileTimeState::find_template_def(primary_decl, rendered_name)` when
  metadata is present, and to use rendered-name lookup only when no structured
  primary exists.
- Routed consteval-template precheck, specialization-key construction, and
  `record_deferred_instance` through the resolved structured primary.
- Kept `Module::find_template_def_by_rendered_preservation_name` as
  preservation/deferred bookkeeping only after the structured compile-time
  state check has succeeded.
- Added focused HIR coverage proving a stale rendered template name cannot
  invoke deferred instantiation after a complete structured primary miss.

## Suggested Next

Execute Step 3 by fencing the remaining template-registry authority callers in
deduction, collection, seed, and retry paths.

## Watchouts

- Do not treat rendered template spelling as semantic authority after a
  metadata-rich structured miss.
- Do not downgrade supported generated-template routes or rewrite expectations
  as a substitute for capability repair.
- Keep retained string-only registry paths explicitly classified with owner,
  limitation, and removal condition.
- `src/frontend/hir/impl/templates/deduction.cpp` still enters
  `try_infer_template_call_result_for_deduction` through rendered
  `find_template_def(call_node->left->name)` before structured binding work.
- `src/frontend/hir/hir_build.cpp` still has rendered primary lookups for
  specialization collection, seed recording, instantiation collection,
  consteval-template collection, and deferred-template retry.
- The builtin `"forward"` helper remains a rendered no-metadata compatibility
  bridge until that generated helper has a structured declaration carrier.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains `frontend_hir_tests` passing 1/1.
