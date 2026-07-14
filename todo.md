# Current Packet

Status: Active
Source Idea Path: ideas/open/771_lir_automatic_local_label_address_table_initializer_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map the automatic local initializer authority handoff

## Just Finished

- Step 1 mapping identified the smallest generic automatic initializer seam:
  local array initialization becomes per-element `AssignExpr` nodes, whose
  `StmtEmitter::emit_set_assign_value` consumer in
  `src/codegen/lir/hir_to_lir/lvalue.cpp` calls string `coerce(rhs.str(), ...)`
  before `emit_store_assignable_value`. That text-only handoff drops nested
  `LabelAddrExpr` `DirectConstant` authority before the local-table element
  store. The next packet must preserve a pointer-typed direct constant from
  the assignment RHS into its pointer element store, retaining current-function
  owner, target block, and produced-value identity.

## Suggested Next

- Implement the mapped `emit_set_assign_value` pointer-store handoff only,
  then add a dedicated `frontend_lir_automatic_local_label_address_table_initializer`
  positive/malformed probe. Positively require two nested `&&label` elements
  to store native direct values through generic indexed local slots; malformed
  cases must reject raw text plus invalid/foreign owner or target, non-pointer
  type, and missing/invalid/foreign produced identity at that boundary.

## Watchouts

- This is initializer-element production only: exclude later table `DeclRef`
  decay, carrier/`IndirBrStmt`, verifier, backend, Raw-BIR/importer, 767, and
  769. Do not use synthetic bridges, raw-text recovery, testcase routing, or
  expectation downgrades. The `LocalDecl` scalar path is not the generic
  array-element consumer seam.

## Proof

- Mapping only; no code proof ran in this packet. Boundary evidence is the
  already observed fresh `ctest --test-dir build -j --output-on-failure -R
  '^frontend_lir_'` result: 6/7 pass and `frontend_lir_call_type_ref` fails
  with `LirStoreOp.val: must not be empty`, consistent with its automatic local
  `void *table[] = { &&first, &&second };` initializer route. Do not replace
  test logs during this mapping packet.
