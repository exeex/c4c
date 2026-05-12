Status: Active
Source Idea Path: ideas/open/201_hir_template_registry_structured_generated_paths.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Record Closure Evidence

# Current Packet

## Just Finished

Step 4: Record Closure Evidence.

Status note: this packet records the final evidence for
`ideas/open/201_hir_template_registry_structured_generated_paths.md`. The HIR
template-registry blocker recorded against idea 195 is cleared for the
inventoried HIR hot files, subject only to the explicit compatibility leftovers
listed below. This does not claim broader frontend-to-BIR closure or backend
restart readiness beyond this blocker.

Final caller classification:

- `src/frontend/hir/impl/expr/call.cpp`: generated-call lowering is
  metadata-rich semantic authority when `function_decl_nodes_` supplies a
  declaration carrier. Explicit and deduced generated-template calls now use
  structured `find_template_def(declaration, rendered_name)` and fail closed
  after a complete structured miss. Rendered lookup remains only for
  no-declaration compatibility, including the builtin `"forward"` helper.
- `src/frontend/hir/impl/templates/deduction.cpp`: template deduction result
  inference is metadata-rich when it has an AST declaration node. The
  deduction path uses structured template definition lookup from that
  declaration carrier; rendered spelling is not semantic recovery after
  structured identity is available and missed.
- `src/frontend/hir/hir_build.cpp`: generated template collection and seed
  paths use `function_decl_nodes_` as the structured declaration carrier when
  available. Rendered template definition lookup is retained only for
  no-carrier collection/seed compatibility.
- `src/frontend/hir/impl/compile_time/engine.cpp`: pending consteval replay and
  deferred retry bookkeeping use `TemplateCallInfo::primary_template_decl` /
  resolved primary definition identity when available. The replay path copies
  template-call metadata before deferred instantiation so registry lookups do
  not read stale HIR pool references after instantiation growth.

Blocker clearance:

- Cleared: no known metadata-rich generated template-registry path in
  `call.cpp`, `deduction.cpp`, `hir_build.cpp`, or `engine.cpp` can recover
  semantic authority through string-only `find_template_def(name)` or
  `has_template_def(name)` after structured template identity is available and
  misses.
- Remaining work for idea 195 is outside this blocker and must be decided by
  the supervisor/plan-owner lifecycle after this idea is validated or closed.

## Suggested Next

Proceed to Step 5 validation and closure handoff for
`ideas/open/201_hir_template_registry_structured_generated_paths.md`.

## Watchouts

- Retained compatibility: `src/frontend/hir/impl/expr/call.cpp` keeps rendered
  lookup for generated-call sites with no `function_decl_nodes_` declaration
  carrier. Owner: HIR generated-call lowering. Limitation: the path cannot
  prove the primary template declaration. Removal condition: all generated-call
  lowering inputs carry a declaration identity, including the builtin
  `"forward"` helper bridge.
- Retained compatibility: `src/frontend/hir/hir_build.cpp` permits rendered
  `find_template_def(name)` only when `function_decl_nodes_` has no declaration
  carrier for the template name. Owner: HIR build collection/seed scaffolding.
  Limitation: no structured declaration identity is available. Removal
  condition: all generated template collection/seed callers carry a primary
  declaration identity instead of only rendered spelling.
- Retained compatibility: `src/frontend/hir/impl/compile_time/engine.cpp`
  still uses rendered compile-time lookup only when `TemplateCallInfo` has no
  `primary_template_decl`. Owner: compile-time replay of legacy/no-metadata HIR
  template calls. Limitation: old metadata cannot prove the primary
  declaration. Removal condition: generated HIR template calls require or
  synthesize `primary_template_decl`.
- Retained compatibility: `CompileTimeState::record_deferred_instance` fills a
  missing `primary_def` from rendered spelling when the caller supplies no
  primary. Owner: compile-time registry bookkeeping. Limitation: only
  no-metadata callers use this branch after replay precheck passes the resolved
  primary. Removal condition: deferred-instantiation APIs require the primary
  template declaration.

## Proof

Evidence-only `todo.md` update. Per delegated proof contract, no build or test
proof was run and no root-level log was created or modified for this packet.
