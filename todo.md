# Current Packet

Status: Active
Source Idea Path: ideas/open/767_lir_computed_goto_table_element_pointer_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract direct frontend-LIR capability probes

## Just Finished

- Completed 767 Step 2 source-form inventory:
  - Static-local pointer table: HIR promotes the table to a global with a
    `LinkNameId`; `emit_lval_dispatch` returns its base as a string,
    `emit_indexed_gep(string, string)` emits a raw-result `LirGepOp`, and
    `emit_rval_from_access_ptr` emits the element `LirLoadOp` with a raw result.
    The global input authority first disappears at the string lvalue/GEP seam.
  - Local pointer table: the table starts as a local-slot string;
    `emit_lval_dispatch` and `emit_indexed_gep(string, string)` retain only
    presentation, then `emit_rval_from_access_ptr` emits a raw-result element
    load. The local address has no structured pointer-result carrier before
    the same string GEP/load boundary.
- Both forms have specific producer/result contracts; neither involves
  `IndirBrStmt` or `addr_value` publication.

## Suggested Next

- Extract one direct frontend-LIR production capability probe for each
  static-local and local table-element producer/result contract, including the
  nearby malformed authority rejection. Do not repair or change publication.

## Watchouts

- The statement seam already copies `addr.value_id()`: do not patch
  `IndirBrStmt`/`addr_value`, weaken a verifier, or derive authority from text.
- Use frontend-LIR direct production tests; backend/case and the four external
  cases are integration proof only. Do not touch Raw-BIR/importer, 734, or
  accepted 765/766 work.
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
