Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Split The Narrow Publication Owner

# Current Packet

## Just Finished

Completed Step 3 implementation for the narrow RV64 consumer/emission owner.
The fix stayed in `src/backend/mir/riscv/codegen/object_emission.cpp`; the
prepared producer and `prepared_local_memory_emit.cpp` did not need changes.

Implemented behavior:

- Added a strict pointer/global store-local publication fragment that consumes
  an existing available `StoreLocalPublication` plus same-instruction
  `DirectGlobal` address materialization and emits the global object's address
  directly into the exact prepared frame slot.
- Tightened `prepared_memory_access_for_local_instruction(...)` so an indexed
  memory access is accepted only when its result/stored value identity matches
  the `LoadLocalInst`/`StoreLocalInst`; otherwise it falls back to the unique
  value-based lookup. This prevents memory-access index collisions from
  selecting the following direct-global access for a local load.
- Added nonvolatile dead `LoadLocalInst` elision based on an explicit BIR
  use-after scan. This covers the representative `%t33 = bir.load_local ptr
  %lv.l` after `*l = 0` has already been represented as direct
  `bir.store_global @f, i16 0`, without accepting live unsupported local loads.
- Refined the use-after scan so it also treats `MemoryAddress::base_value`
  pointer operands on later `LoadLocalInst`, `LoadGlobalInst`,
  `StoreLocalInst`, and `StoreGlobalInst` as uses. `PhiInst` incoming values are
  checked too, although the prepared object route should have removed phi nodes
  before this consumer.
- Added focused fail-closed coverage for a live reload of a direct-global local
  pointer publication: the negative case stores a direct global address into a
  local pointer slot, reloads it, and uses that reloaded pointer as a later
  memory-address base. RV64 object emission now rejects this live publication
  reload instead of relying on dead-load elision.
- Added focused coverage in
  `tests/backend/case/riscv64_pointer_global_local_publication.c` plus
  `backend_dump_riscv64_pointer_global_local_publication`,
  `backend_cli_riscv64_pointer_global_local_publication`, and
  `backend_cli_failure_riscv64_pointer_global_local_publication_live_load_rejection`.

Fail-closed behavior preserved: the new publication fragment rejects missing or
ambiguous publication records, non-direct or missing global identity, TLS or
non-default address-space materializations, mismatched destination access,
mismatched source value, non-frame-slot destinations, non-8-byte pointer stores,
non-base-plus-offset frame slots, and scalar-only direct-global/local facts.
Dead-load elision is limited to nonvolatile loads whose result has no later BIR
value use or memory-address base use.
Live reloads of direct-global local pointer publications remain fail-closed
until a separate authority exists for consuming the reloaded pointer as a memory
base.

## Suggested Next

Proceed to Step 4 representative integration proof. Use the new focused tests
and inspect the representative `pr57861.c` object/disassembly to decide whether
idea 649 is complete or whether a downstream non-publication owner remains.

## Watchouts

- Do not treat scalar frame-slot local-memory facts from idea 640 as
  pointer/global local-publication authority.
- Do not reopen direct global-symbol local-memory support from idea 631 or
  generic pointer freshness from idea 600 unless fresh evidence proves a new
  local-publication boundary.
- Do not use the `main` call-argument direct-global select-chain evidence as
  the owner for this idea; the representative owner is inside `foo` around
  `%lv.l`.
- Do not rewrite prepared provenance or mark all unknown local pointer slots as
  supported. The discovered positive route depends on exact publication,
  direct-global identity, slot identity, and ordering.
- The object route now emits `pr57861.c` successfully as an object, but Step 4
  should still inspect representative disassembly and avoid treating unrelated
  downstream codegen quality as part of this publication slice.
- Do not infer authority from source spelling, final assembly order,
  diagnostics, testcase identity, local/global names, or stack-slot shape.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Proof passed and was written to `test_after.log`:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_(dump|cli|cli_failure)_riscv64_pointer_global_local_publication" && mkdir -p build/agent_state/649_step3_pointer_global_local && build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu --codegen obj tests/c/external/gcc_torture/src/pr57861.c -o build/agent_state/649_step3_pointer_global_local/pr57861.o; } 2>&1 | tee test_after.log'
```

Result: build succeeded, the positive dump/object tests and expected-failure
object test passed, and representative
`pr57861.c` emitted
`build/agent_state/649_step3_pointer_global_local/pr57861.o`.
The focused object test now also asserts the expected RV64 byte sequence for
`auipc t1, 0; mv t1, t1; sd t1, 0(sp)` via `EXPECTED_HEX_CONTAINS`.
