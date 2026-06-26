Status: Active
Source Idea Path: ideas/open/384_prepared_global_symbol_memory_access_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Global-Symbol Memory-Access Facts

# Current Packet

## Just Finished

Step 3 canonicalized aggregate `LirOperandKind::Global` load lanes in
`src/backend/bir/lir_to_bir/memory/local_slots.cpp` to emit
`bir::LoadGlobalInst` with authoritative `global_name`, `global_name_id`,
byte offset, and alignment, then store the lane value into the aggregate leaf
slot.

The previous `LoadLocalInst` plus prepared `GlobalSymbol` address fact shape is
no longer used for supportable aggregate global lanes, so RV64 object emission
can consume the existing prepared global-load path without raw spelling
inference. The focused prepared-stack-layout contract now expects the supported
lane as `LoadGlobalInst`, while the raw/no-id `LoadLocalInst` fixture remains
fail-closed.

## Suggested Next

Run the representative `src/20030914-2.c` RV64 torture proof again to confirm
the canonicalized producer shape reaches the object route, then classify the
next residual boundary.

## Watchouts

- Raw/no-id `LoadLocalInst` global lanes intentionally remain unpublished and
  fail-closed.
- RV64 object emission should continue consuming explicit `LoadGlobalInst`
  prepared facts and must not infer global-symbol memory facts from raw
  `LoadLocalInst` spelling.
- Keep `plan.md` and this `todo.md` pointed at
  `ideas/open/384_prepared_global_symbol_memory_access_publication.md`.

## Proof

Delegated proof ran exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$'; } > test_after.log 2>&1
```

Result: passed. Test subset: `backend_prepare_stack_layout` and
`backend_riscv_object_emission`. Proof log: `test_after.log`.
