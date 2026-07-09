Status: Active
Source Idea Path: ideas/open/619_bir_aggregate_global_store_handoff.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair BIR Or Prepared Aggregate Global Handoff

# Current Packet

## Just Finished

Step 3 - Repair BIR Or Prepared Aggregate Global Handoff completed as a BIR
semantic producer repair.

Changed files:

- `src/backend/bir/lir_to_bir/aggregate.cpp`
- `src/backend/bir/lir_to_bir/lowering.hpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `tests/backend/CMakeLists.txt`
- `tests/backend/case/aggregate_global_store_handoff.c`
- `todo.md`

The repair adds a general aggregate-copy-to-global helper and admits whole
aggregate stores with direct global destinations or resolved global-pointer
destinations. Source aggregate authority still comes from existing
`aggregate_value_aliases_`/local aggregate slots, so the rule covers compound
literals, local aggregate objects, and call-result aggregate slots without
adding named-case shortcuts or widening ordinary local-memory store admission.

Focused probes were refreshed under
`build/agent_state/619_step3_repair_aggregate_global_handoff/`:

| Row | `--dump-bir` | `--dump-prepared-bir` | Result |
| --- | --- | --- | --- |
| `src/pr22141-1.c` | rc `0` | rc `0` | Direct compound-literal-to-global assignment now emits leaf `bir.store_global @u` stores. |
| `src/compndlit-1.c` | rc `0` | rc `0` | Bitfield compound-literal assignment now emits `bir.store_global @x`. |
| `src/pr57344-1.c` | rc `0` | rc `0` | Local aggregate assignment to `s[1]` now emits offset `bir.store_global @s` stores. |
| `src/pr39120.c` | rc `0` | rc `0` | Aggregate call result assignment now emits `bir.store_global @x` from the preserved sret slot. |
| `src/20020225-2.c` | rc `0` | rc `0` | Local-only aggregate guard retained prior semantic/prepared BIR behavior. |
| `src/ieee/mul-subnormal-single-1.c` | rc `0` | rc `0` | Local-only aggregate guard retained prior semantic/prepared BIR behavior. |

Added focused backend test
`backend_dump_riscv64_aggregate_global_store_handoff`, which checks semantic
BIR leaf `store_global` facts for direct global, global array element, and call
result aggregate handoff.

## Suggested Next

Execute Step 4 as a no-code classification packet: rerun or cite the refreshed
backend object scan for the Step 1 row set and classify any residual stops now
that semantic/prepared aggregate-global handoff is present.

## Watchouts

- The repair emits scalar leaf `StoreGlobalInst` facts into linear global
  storage; it does not add aggregate-width store instructions or RV64 consumer
  reconstruction.
- `src/ieee/20001122-1.c` and any other residual prepared-global-data/RV64
  consumer rows still need explicit owner classification instead of being
  counted as semantic producer gaps.
- Keep `src/20020225-2.c` and `src/ieee/mul-subnormal-single-1.c` in the guard
  surface for follow-up validation because this packet intentionally reused
  local aggregate source slots while changing only the destination handoff.

## Proof

Focused diagnostic probes:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu <case>`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu <case>`
- `ctest --test-dir build -j --output-on-failure -R '^backend_dump_riscv64_aggregate_global_store_handoff$'`

Delegated proof command:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: pass. `test_after.log` reports `100% tests passed, 0 tests failed out
of 347`.
