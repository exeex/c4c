Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Fence Deduction, Collection, Seed, And Retry Paths

# Current Packet

## Just Finished

Step 3: Fence Deduction, Collection, Seed, And Retry Paths.

Status note: this packet finished the remaining generated-call lowering fence
in `src/frontend/hir/impl/expr/call.cpp`. Step 3 is closure-complete for the
known hot files; Step 4 can now record final closure evidence for idea 195's
HIR template-registry blocker.

Changed files:

- `src/frontend/hir/impl/expr/call.cpp`
- `tests/frontend/frontend_hir_tests.cpp`

Completed work:

- Added a local `TemplateDefLookup` path for generated-call lowering that
  consults `function_decl_nodes_` first and calls structured
  `find_template_def(declaration, rendered_name)` whenever a declaration
  carrier exists.
- Replaced the explicit-template branch's rendered
  `has_template_def(n->left->name)` / `find_template_def(n->left->name)`
  authority with the structured lookup result. A complete structured miss now
  throws the existing no-viable-template diagnostic instead of falling through
  to rendered spelling.
- Replaced the deduced-template branch's rendered
  `find_template_def(n->left->name)` authority with the same structured lookup
  and fail-closed behavior.
- Preserved rendered lookup only for no-declaration compatibility, including
  the existing builtin `"forward"` bridge.
- Added focused HIR coverage for explicit and deduced generated-call lowering
  with stale rendered spelling plus a missing structured declaration carrier.

## Suggested Next

Proceed to Step 4 closure evidence for
`ideas/open/201_hir_template_registry_structured_generated_paths.md`.

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
- No known metadata-rich generated template-registry path in the inventoried
  hot files still recovers semantic authority through rendered
  `find_template_def(name)` after structured identity is available and missed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests)$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains `frontend_hir_tests` and
`frontend_hir_lookup_tests` passing 2/2.
