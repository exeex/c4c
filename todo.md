# Current Packet

Status: Active
Source Idea Path: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bind probes and select the narrowest generic seam

## Just Finished

- Completed 767 Step 3 no-code probe-harness record in
  `tests/frontend/frontend_lir_call_type_ref_test.cpp`:
  - Static-local table harness: extend `test_global_array_gep_identity_contract`
    with `int f(int i) { static const void *t[] = {&&a, &&b}; goto *t[i];
    a: return 1; b: return 2; }`. Future positive: the global-table element
    GEP retains its `LinkNameId`, typed i64 index authority, and pointer result
    ID; the following pointer load consumes that exact GEP ID and returns its
    own valid pointer ID. Future malformed checks: missing/invalid GEP result,
    missing global base ID, raw/invalid index, and load pointer that is
    foreign or does not identify the GEP result.
  - Local table harness: add alongside
    `test_local_and_parameter_rvalue_identity_route` a minimal
    `int f(int i) { void *t[] = {&&a, &&b}; goto *t[i]; a: return 1;
    b: return 2; }`. Future positive: the local table-address/result path
    supplies a current-function pointer ID to the typed table-element GEP;
    its pointer load consumes that exact ID and returns a valid pointer ID.
    Future malformed checks: raw/invalid/foreign/non-pointer local GEP base,
    raw index, missing GEP result, and a load pointer not selected by that GEP.
- These are future contracts, not current positives: both existing paths lose
  structured result authority at the string GEP/load boundary. They do not
  relax or accept raw behavior, and neither asserts indirect-branch publication.

## Suggested Next

- Bind the two future probe contracts to their producer/result maps and select
  the narrowest generic implementation owner, or return a precise blocker.
  Do not repair or change publication in this decomposition packet.

## Watchouts

- The statement seam already copies `addr.value_id()`: do not patch
  `IndirBrStmt`/`addr_value`, weaken a verifier, or derive authority from text.
- Use frontend-LIR direct production tests; backend/case and the four external
  cases are integration proof only. Do not touch Raw-BIR/importer, 734, or
  accepted 765/766 work.
- A positive capability probe cannot pass until the Step 4-selected generic
  producer/result seam is implemented in the distinct Step 5 packet. Do not
  weaken expectations, accept raw behavior, or return to 764 beforehand.
- Evidence map: `src/codegen/lir/hir_to_lir/lvalue.cpp`:
  `emit_lval_dispatch`, string `emit_indexed_gep`, and
  `emit_rval_from_access_ptr`; `src/codegen/lir/hir_to_lir/expr/misc.cpp`:
  `emit_rval_payload(IndexExpr)`. AST-backed query tooling was used for the
  C++ translation units; HIR observation confirmed static-local global versus
  local-slot source classification.

## Proof

- Step 1 ran fresh `cmake --build --preset default` successfully, then exactly
  `ctest --test-dir build -j --output-on-failure -R '^(llvm_gcc_c_torture_src_comp_goto_1_c|llvm_gcc_c_torture_src_20040302_1_c|llvm_gcc_c_torture_src_20041214_1_c|llvm_gcc_c_torture_src_920501_4_c|llvm_gcc_c_torture_src_920501_5_c)$'`;
  `comp-goto-1` passed and the remaining four failed at the same missing
  carrier authority. No canonical test logs were overwritten.
- Step 2 was read-only: source-form observation used HIR output and the direct
  production symbols above; no tests or logs were changed.
- Step 3 proof is a contract/harness record only. Passing positive and
  malformed focused proof is deferred to the post-selection implementation
  packet, followed by an explicit lifecycle handoff to 764.
