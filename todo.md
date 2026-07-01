Status: Active
Source Idea Path: ideas/open/500_semantic_global_static_gep_admission_producer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route The First Global/Static GEP Packet

# Current Packet

## Just Finished

Step 3 routed the prerequisite `global_static_gep_authority` certificate
surface instead of implementing it in this packet. The exact blocker and route
are recorded in
`build/agent_state/500_step3_global_static_gep_authority/summary.md`: the
producer inputs are visible in global GEP lowering (`memory/addressing.cpp`)
and lowerer state/declarations (`lowering.hpp`), which were outside this
packet's owned files.

## Suggested Next

Run a follow-up implementation packet that owns
`src/backend/bir/lir_to_bir/lowering.hpp` and
`src/backend/bir/lir_to_bir/memory/addressing.cpp` in addition to the BIR
record, publication, and focused backend test files. That packet should publish
production `global_static_gep_authority` records during global GEP lowering and
still leave final semantic global/static GEP admission fail-closed for a later
consumer packet.

## Watchouts

- This is a BIR semantic producer packet, not RV64/MIR lowering.
- Do not infer global/static GEP authority from raw shape, names, labels, final
  homes, object files, relocations, lowered target behavior, or route-local
  slots.
- Keep `src/20051104-1.c` fail-closed or route it to string/global-pointer
  provenance first; direct global-object GEP authority does not prove the
  pointed string-literal object behind `s.name[s.len]`.
- Keep `src/ieee/copysign2.c` fail-closed or route it through runtime/string
  intrinsic consumer authority before admitting its static-array GEP use.
- Existing `bir::Global`, `LoadGlobalInst`/`StoreGlobalInst`, and lowerer
  `GlobalAddress` helpers are useful lower surfaces, but there is not yet a
  durable certificate tying global object, layout path, dynamic range, element
  byte range, and LIR producer coordinate together.
- Preserve final admission fail-closed for all rows until a matching available
  `global_static_gep_authority` record exists.
- Do not try to reconstruct the certificate later in `publication_plans.cpp`
  from `bir::Global`, global loads/stores, prepared object data, final homes,
  or target lowering; the LIR GEP path and coordinate must be captured while
  global GEP lowering still has them.

## Proof

Step 3 route proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
