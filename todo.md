Status: Active
Source Idea Path: ideas/open/843_lir_direct_hir_family_construction_array_composition.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected runtime-text construction escape hatches

# Current Packet

## Just Finished

Completed plan.md Step 3 for the selected indexed local-array GEP producer.

Proved recursive array composition and GEP typed-element consumption for the
selected static local-array GEP route:

- Added recursive `TypeSpec` to `LirTypeRef::array` construction for selected
  local array object authority.
- Updated `StmtEmitter::indexed_gep_elem_ty` so supported indexed array element
  refs use structured `LirTypeRef::array` facts instead of
  `hir_rendered_indexed_gep_element_type_text`.
- Extended the focused local immediate indexed-array fixture to nested static
  arrays and asserted that the selected GEP element type carries structured
  array shape facts.
- Added stale compatibility-text proof: mutating both selected GEP array text
  mirrors to `[99 x i8]` still verifies because equality/rendering consume the
  structured recursive array facts.

## Suggested Next

Execute plan.md Step 4. Check whether any selected indexed local-array GEP
runtime-text construction escape hatch can now be retired without deleting
fallbacks required by unsupported shapes or unrelated consumers.

## Watchouts

Keep this route to producer construction and recursive array composition.
Global/extern mirrors, collector scans, Raw-BIR receiver packets, and terminal
compatibility deletion belong to later ordered successors unless an exact
selected producer proves named-consumer parity.

The code intentionally duplicates a small recursive array-ref helper in the
local object authority and indexed GEP producer surfaces. A later cleanup can
centralize it only if that does not widen this route into global/extern,
collector, or Raw-BIR ownership.

Do not delete the runtime-text fallback for unsupported indexed GEP shapes.
Keep global array GEP identity and local-array backend publication rows out of
Step 4 unless the proof names one exact selected consumer.

## Proof

Step 3 proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$|^frontend_lir_extern_decl_type_ref$'; } > test_after.log 2>&1`

Result: passed.

Focused subset: `frontend_lir_call_type_ref` and
`frontend_lir_extern_decl_type_ref`, 2/2 passing.

Log path: `test_after.log`.
