# LIR To BIR Local Memory Admission Todo

Status: Active
Source Idea Path: ideas/open/297_lir_to_bir_local_memory_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Isolate The Admission Failure

# Current Packet

## Just Finished

Step 1 diagnosis completed for focused idea 297. Existing
`test_after.log` shows the focused cases fail before prepared handoff as
`gep local-memory semantic family` (`00143`, `00157`, `00176`, `00181`,
`00182`, `00185`, `00195`, `00205`, `00209`), `store local-memory semantic
family` (`00046`, `00140`), or `load local-memory semantic family` (`00216`,
`00218`). `00204` remains excluded: its diagnostic is the separate bootstrap
global aggregate/array gate, not this local-memory admission family.

Representative dump probes:

- `00143`: `--codegen llvm` shows the first rejected form is dynamic GEP on a
  scalar local array, e.g. `short a[39]; int n; a[n] = ...` lowers to
  `getelementptr i16, ptr %lv.a, i64 %t8`. A reduced probe with
  `short a[39]; int n=0; a[n]=1;` fails `--dump-bir` in
  `gep local-memory semantic family`, while pointer increment from a stored
  local array base (`short *p=a; p++;`) already lowers.
- `00046`: reduced probes show `v.a=1` and `v.b1=2` lower, but adding
  `v.c=3` through nested anonymous struct/union subobjects fails
  `store local-memory semantic family`. The semantic shape is a scalar store
  through a GEP-derived local aggregate subobject whose address state reaches
  `LocalSlotAddress`/provenance checks but is not admitted for the nested
  aggregate byte-offset path.
- `00216`: reduced probes show direct scalar field access from a pointer
  parameter (`return p->s.a`) lowers, but aggregate load/copy from
  pointer-derived local memory (`struct S x = p->s`, `x = p->s`, or
  `struct S x = *q`) fails `load local-memory semantic family`. The semantic
  shape is aggregate load from a pointer/local-address provenance source, not
  from a `local_aggregate_slots_` name.

Likely owner surfaces:

- `src/backend/bir/lir_to_bir/memory/addressing.cpp`:
  `BirFunctionLowerer::lower_memory_gep_inst` delegates local arrays through
  `try_lower_local_array_slot_gep`.
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`:
  `try_lower_local_array_slot_gep` currently rejects dynamic non-pointer
  element arrays; `try_lower_local_pointer_array_base_gep` already contains the
  nearby dynamic scalar-array admission pattern using
  `DynamicLocalAggregateArrayAccess`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp` and
  `src/backend/bir/lir_to_bir/memory/provenance.cpp`: aggregate
  load/store boundaries are rejected around `lower_memory_load_inst`,
  `lower_memory_store_inst`, `try_lower_pointer_provenance_load/store`, and
  the `can_address_scalar_subobject`/aggregate-copy helpers when the pointer
  is represented by local address provenance rather than a named aggregate
  slot.

## Suggested Next

Execute Step 2 with the GEP owner first: teach
`try_lower_local_array_slot_gep` to admit dynamic scalar local-array GEPs by
publishing a `DynamicLocalAggregateArrayAccess`/equivalent local-memory
address state for non-pointer scalar element arrays, matching the existing
`try_lower_local_pointer_array_base_gep` dynamic scalar path. Prove against
`00143` first, then the remaining focused GEP cases; leave aggregate
store/load boundary repair for Step 3 unless the same helper change directly
unblocks them.

## Watchouts

- Focused GEP local-memory cases: `00143`, `00157`, `00176`, `00181`,
  `00182`, `00185`, `00195`, `00205`, `00209`.
- Store/load boundary checks: `00046`, `00140`, `00216`, `00218`.
- `00204` is a separate bootstrap global aggregate/array semantics gate; do
  not fold it into this owner without evidence.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.
- Do not claim progress through filename matching or named-case shortcuts.
- Step 2 should not special-case `00143`; the rejected form is semantic:
  dynamic GEP into scalar local arrays whose elements are represented as local
  slots.
- Store/load boundary notes for Step 3: `00046` needs nested aggregate
  subobject scalar stores through local address provenance; `00216` needs
  aggregate load/copy from pointer-derived local memory.

## Proof

No broad backend regex or canonical proof logs were rerun or modified.
Diagnostic-only commands run:

- `rg` over existing `test_after.log` for focused case diagnostics.
- `./build/c4cll --codegen llvm --target aarch64-linux-gnu` for
  `00143`, `00046`, and `00216` to inspect representative IR shape.
- `./build/c4cll --dump-bir --target aarch64-linux-gnu` for focused
  representatives and reduced process-substitution probes:
  `00143`, `00046`, `00216`, dynamic local-array GEP, local pointer increment,
  nested anonymous aggregate stores, and aggregate loads from pointer-derived
  local memory.
