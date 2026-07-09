Status: Active
Source Idea Path: ideas/open/650_edge_store_local_aggregate_publication_ordering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Representative Edge-Store Evidence

# Current Packet

## Just Finished

Completed Step 1 diagnostic refresh for `pr68185.c` and `pr68321.c`.

Artifacts were written under
`build/agent_state/650_step1_edge_store_evidence/`, including semantic BIR,
prepared BIR, MIR, MIR trace, RV64 asm/object diagnostics, focused summaries,
and `summary.md`.

Current classification: both representatives still point at the idea 650
family for the RV64 object route. The first object-route owner is edge-store
local publication ordering / destination-access authority for out-of-SSA
edge-store carriers, not scalar frame-slot lookup, direct global-symbol local
memory, aggregate global-object materialization, or stack-home policy.

`pr68185.c` evidence:

- Object diagnostic:
  `unsupported_local_memory_access ... function=main; block=logic.rhs.end.33; block_index=23; instruction_index=0; access_base=none`.
- Predecessor edges: `logic.rhs.end.33 -> logic.end.34` and
  `logic.skip.32 -> logic.end.34`.
- Edge-store destination/carrier: `%t38.phi`.
- Consumer point: `%t38 = bir.load_local i32 %t38.phi` in `logic.end.34`.
- Source values: `%t36` on `logic.rhs.end.33`, immediate `0` on
  `logic.skip.32`.
- Available authority: prepared join transfer reports `carrier=edge_store_slot`
  with `ownership=authoritative_branch_pair`; parallel-copy records exist for
  both predecessor edges.
- Missing authority: store-source publications for both edge stores report
  `status=missing_destination_access` with `intent=store_local_publication`,
  so RV64 local-memory lookup sees `access_base=none`.
- Non-owner local slots: `%lv.h` and `%lv.g` already have scalar frame-slot
  accesses and same-block source freshness where relevant.

`pr68321.c` evidence:

- Object diagnostic:
  `unsupported_local_memory_access ... function=fn1; block=logic.rhs.end.12; block_index=11; instruction_index=0; access_base=none`.
- Predecessor edges: `logic.rhs.end.12 -> logic.end.13` and
  `logic.skip.11 -> logic.end.13`.
- Edge-store destination/carrier: `%t17.phi`.
- Consumer point: `%t17 = bir.load_local i32 %t17.phi` in `logic.end.13`.
- Source values: `%t15` on `logic.rhs.end.12`, immediate `1` on
  `logic.skip.11`.
- Available authority: prepared join transfer reports `carrier=edge_store_slot`
  with `ownership=authoritative_branch_pair`; parallel-copy records exist for
  both predecessor edges.
- Missing authority: store-source publications for both edge stores report
  `status=missing_destination_access` with `intent=store_local_publication`,
  so RV64 local-memory lookup sees `access_base=none`.
- Non-owner local aggregate/frame-slot read: `%t20 = bir.load_local i32
  %lv.g.0, addr %lv.g.0` has a scalar frame-slot access; it is downstream of
  the edge-store owner and is not the first missing authority.

The RV64 asm route for both representatives also reports
`riscv prepared module emitter does not support this prepared global storage
layout`; the object-route diagnostic reaches the edge-store local-memory
boundary above and is the focused Step 1 owner.

## Suggested Next

Proceed to Step 2 by locating the narrow producer or consumer boundary for
edge-store carrier destination-access authority. Start from
`build/agent_state/650_step1_edge_store_evidence/summary.md`, then inspect
prepared publication production for `%t38.phi` / `%t17.phi` and the RV64
`StoreLocalInst` access admission path.

## Watchouts

- Preserve the distinction between authoritative branch-pair join-transfer
  facts and a usable RV64 local-memory destination access; the former exists,
  the latter is missing for the edge-store carrier stores.
- Do not treat ordinary scalar frame-slot accesses for `%lv.g`, `%lv.h`,
  `%lv.g.0`, or `%lv.c` as the missing edge-store authority.
- Do not reopen direct global-symbol local memory from idea 631.
- Do not absorb aggregate global-object materialization from idea 641 or
  aggregate/sret/byval stack-home policy from idea 633 unless later evidence
  proves a distinct downstream owner.
- The initial `commands.log` entries with `rc=127` came from a broken capture
  wrapper and were immediately rerun with the fixed wrapper; use the second
  command set as evidence.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No pass/fail proof was required or run for this diagnostic packet, and
`test_after.log` was not overwritten.

Diagnostic commands and outputs were captured under
`build/agent_state/650_step1_edge_store_evidence/`. The fixed diagnostic set
ran:

```sh
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c
build/c4cll --dump-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c
build/c4cll --trace-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c
build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c -o build/agent_state/650_step1_edge_store_evidence/pr68185.s
build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68185.c -o build/agent_state/650_step1_edge_store_evidence/pr68185.o
build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c
build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c
build/c4cll --dump-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c
build/c4cll --trace-mir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c
build/c4cll --codegen asm --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c -o build/agent_state/650_step1_edge_store_evidence/pr68321.s
build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/pr68321.c -o build/agent_state/650_step1_edge_store_evidence/pr68321.o
```

The BIR/prepared/MIR diagnostics exited 0. The asm/object commands exited
nonzero as expected diagnostic evidence and captured the current RV64 route
blockers.
