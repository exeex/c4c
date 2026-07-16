Status: Active
Source Idea Path: ideas/open/843_lir_direct_hir_family_construction_array_composition.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add direct family construction for the selected producer

# Current Packet

## Just Finished

Completed plan.md Step 1 inventory for HIR construction authority users.

Selected Step 2 producer target: the indexed local-array GEP element-type
construction path in `StmtEmitter::indexed_gep_elem_ty` /
`StmtEmitter::emit_indexed_gep`.

Current authority users and evidence:

- `LirTypeRef::array` already carries recursive array element and length facts
  and renders compatibility text from those facts.
- `llvm_alloca_ty` recursively renders array storage text from `TypeSpec`, but
  the indexed GEP element path still converts array element `TypeSpec` through
  rendered text before returning `hir_rendered_indexed_gep_element_type_text`.
- `LirCurrentFunctionLocalObjectPointer::indexed_element_type`,
  `LirGepOp.local_object_authority`, and
  `LirGepOp.requires_native_local_gep_authority` already provide selected
  local-array GEP receipt and verifier enforcement for native result, base,
  element, and immediate-index authority.
- Existing focused coverage in `frontend_lir_call_type_ref` checks selected
  direct local-array GEP receipt, element-type parity, raw base rejection, raw
  index rejection, mismatched element rejection, stale authority rejection, and
  invalid owner/object/liveness cases.
- Existing `frontend_lir_extern_decl_type_ref` coverage proves
  `LirTypeRef::array` retains structured array semantics and derives
  compatibility text without reparsing.

Admitted Step 2 construction inputs:

- Static local array `TypeSpec` with concrete outer dimensions.
- Recursive array element facts expressible as `LirTypeRef::array`.
- Scalar, pointer, vector, and named aggregate leaf element refs that are
  already representable as direct `LirTypeRef` leaves for the selected GEP
  path.

Rejected or deferred inputs:

- Rendered `llvm_alloca_ty()` or printer text as the source of array semantics.
- Global/extern initializer mirrors, collector scans, Raw-BIR receipt, parser
  adapters, and inline-assembly text.
- Broad `emit_rval_*` or `coerce` migration outside the selected indexed GEP
  producer.
- Runtime-sized or unsupported array shapes without stable `TypeSpec`
  dimensions.

## Suggested Next

Execute plan.md Step 2. Add a bounded helper that constructs the selected
indexed GEP element `LirTypeRef` directly from `TypeSpec` array facts,
preferably reusing `LirTypeRef::array` recursively, and update
`indexed_gep_elem_ty` to avoid `hir_rendered_indexed_gep_element_type_text` for
supported static local-array element shapes.

Suggested proof command for the code packet:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`

## Watchouts

Keep this route to producer construction and recursive array composition.
Global/extern mirrors, collector scans, Raw-BIR receiver packets, and terminal
compatibility deletion belong to later ordered successors unless an exact
selected producer proves named-consumer parity.

Do not delete the runtime-text fallback in Step 2. Keep it for unsupported
shapes and named consumers until the selected producer has positive and
wrong-authority coverage. Avoid taking over global array GEP identity or local
array backend publication rows; this packet is producer element-type
construction only.

## Proof

Step 1 proof command: `git diff --check`

Result: passed.

No `test_after.log` was written because this was an inventory-only packet.
