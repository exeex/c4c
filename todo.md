Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Populate Base Identity and Known Extent Facts

# Current Packet

## Just Finished

Step 3: Populate Base Identity and Known Extent Facts continued for pointer
SSA/value, formal, byval, sret, and explicit unknown-runtime base routes.

Updated existing pointer-carrier producers without changing route admission,
address kind, byte offsets, sizes, alignments, or prepared/prealloc consumer
behavior:

- `PointerAddress` and dynamic pointer-value array side-table records now have
  reusable provenance helpers for `PointerValue`, `UnknownRuntimeBase`, and
  complete object extents.
- pointer-memory access lowering now preserves a non-unknown carrier identity
  instead of overwriting formal/byval/sret/unknown-runtime provenance with a
  generic `PointerValue`; fallback remains `PointerValue` when no stronger
  identity exists.
- non-byval/non-sret formal pointer params seed `FormalParameter` base identity
  with unknown extent, keeping existing pointer-address routing unchanged.
- byval aggregate parameter materialization records `ByvalParameter` identity
  and complete extent from the already selected aggregate layout on emitted
  pointer-value loads.
- sret return-copy stores record `SretParameter` identity and complete extent
  from the existing return ABI layout.
- direct `calloc` pointer carriers record `PointerValue` identity and complete
  extent only when immediate count*stride fits in `size_t`; dynamic alloca,
  runtime global pointer bases, loaded pointer values, and mutable pointer-slot
  reloads remain explicit `UnknownRuntimeBase`.
- GEP-derived pointer-address records and dynamic pointer-value array
  synthesized load/store addresses carry forward existing base provenance and
  publish requested byte ranges while leaving `range_verdict` at
  `UnknownCompatible`.

## Suggested Next

Review whether any remaining Step 3 producers outside the current owned memory
and call-return routes still emit pointer-value memory addresses with blank
provenance; otherwise hand Step 3 to the supervisor for review/escalation into
requested-range proof.

## Watchouts

- The provenance carrier remains passive. No prepared/prealloc consumer gates
  on the new facts in this slice.
- Byval/sret complete extents come only from already selected aggregate/return
  ABI layouts. Formal pointer params and loaded/runtime pointer values keep
  unknown extent.
- This slice still does not prove requested ranges, dynamic array bounds, or
  layout authority; `range_verdict` remains neutral `UnknownCompatible`.
- Dynamic pointer-value array element offsets use the existing arithmetic and
  casts; this packet did not add new overflow rejection or range proof there.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
