# Current Packet

Status: Active
Source Idea Path: ideas/open/851_hir_function_signature_definition_provenance_architecture_blocker.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide the legal successor or no-feasible-route conclusion

## Just Finished

- Step 1 confirmed the complete production `lower_function` caller set:
  `hir_build.cpp:968,970,983,988,990,1064,1070` and
  `impl/stmt/decl.cpp:103,105`. Its signature accepts only the function
  `Node*`, optional name override, and template/NTTP bindings; no caller
  supplies a definition-backed module aggregate fact. `lower_initial_program`
  first collects/registers ordinary aggregate definitions and materialized
  template instances, then lowers free functions, then pending methods; the
  definition registrations are `hir_types.cpp:3581` and
  `impl/templates/struct_instantiation.cpp:566`. Those earlier registrations
  make module refs exist, but provide no legal direct signature provenance to
  `lower_function`. `lower_struct_method` is a separate later path, likewise
  not a direct-fact producer for free-function normalization.

## Suggested Next

- Execute Step 2's no-feasible-route branch: record the deliberate conclusion
  that the current supported function-signature contract has no legal
  pre-normalization direct-fact producer, return 848 Step 2b for its bounded
  no-change disposition, and keep 838 blocked. Do not create an
  implementation successor.

## Watchouts

- Do not retry 848 Step 2b unless a separately scoped successor supplies a
  direct fact before signature normalization.
- Do not use parser/`TypeSpec`/record/tag/owner/text recovery, a `Node*` map,
  test-only injection, `qtype_from` attachment, or LIR work.
- Preserve accepted evidence: 849 Step 1 `109ea13f4`; 848 Step 2a
  `359a9b94b`; 850 lifecycle conclusion `1723df997` with its fresh-build and
  `frontend_hir_tests` baseline.

## Proof

- Lifecycle/architecture transition only. No new build or test claim is made;
  851 may use read-only traces to make its decision.
