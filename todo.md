Status: Active
Source Idea Path: ideas/open/356_rv64_object_route_assembler_backed_prepared_text.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire RV64 Object Emission to the Shared Traversal Stream

# Current Packet

## Just Finished

Step 3 traversal repair completed. RV64 prepared object emission now uses the
shared traversal emitter only when the traversal is a complete BIR-backed event
stream; prepared control-flow streams with unmapped BIR blocks fall back to the
existing block-driven object-emission path instead of failing at the coarse
prepared-module-shape rejection.

The traversal move consumer now lowers prepared register-to-register moves when
the destination is either a normal value home or an explicit prepared ABI
register, covering existing `before_return` register moves without scanning
parallel-copy bundles or adding testcase-specific handling. Unsupported stack,
cycle-temp, non-register, and non-move shapes still fail closed.

This packet eliminated the remaining Step 3 regressions by also lowering the
general prepared move-bundle shape where the source is a rematerializable i32
immediate and the destination is a prepared GPR, including `function_return_abi`
destinations such as `a0`. The failing cases all used that shared
`before_return` move shape, so the fix is semantic move handling rather than
testcase branching.

The successor-entry traversal-copy object-emission unit test still proves the
shared traversal copy path, and `backend_obj_runtime_rv64_return_add` no longer
regresses.

## Suggested Next

Next packet should run the supervisor-selected broader RV64 object-route guard
or continue with Step 4 object-route audit if the broader guard stays green.

## Watchouts

- The current traversal move lowering covers prepared GPR register moves from
  GPR homes or rematerializable i32 immediates. Stack-slot, cycle-temp,
  non-register, non-i32 immediate, and non-move prepared move operations should
  get targeted diagnostics or semantic lowering in later packets.
- `object_emission.cpp` does not scan `control_flow.parallel_copy_bundles` and
  does not call `prepared_object_parallel_copy_event_kind`; keep that boundary.
- No progress is claimed from the reverted/stashed target-local route.
- Do not close 356 while a second production RV64 fixup/relocation path exists
  unreviewed.
- Do not use RV64 fixups/relocations to hide CFG, edge-copy placement,
  select-carrier, value-home, frame, or diagnostic semantics that belong to
  the shared prepared-object contract from 359.
- `c4c-as.cpp::resolve_local_control_flow_labels` may be a valid textual
  assembler concern, but Step 4 must explicitly decide whether it shares
  low-level machinery with object emission or remains assembler-local.
- `src/backend/mir/riscv/assembler/elf_writer.cpp` minimal/pending relocation
  code must not become the second production object route without review.
- `20030216-1.c` is a separate prepared global-storage/data blocker; do not
  mix float/global data support into the stack-slot object-fragment packet.
- The two passing cases are only current allowlist passes. Do not use them as
  proof of broad RV64 object-route completeness.
- Reject testcase-name shortcuts, expectation weakening, target-local CFG
  reconstruction, and silent block/edge dropping.

## Proof

Ran:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_cts_00002$|^backend_obj_runtime_rv64_cts_00012$|^backend_obj_runtime_rv64_return_add_sub_chain$|^backend_riscv_object_emission$' > test_after.log 2>&1
```

Build was green. CTest result in `test_after.log`: 4/4 passed
(`backend_obj_runtime_rv64_cts_00002`,
`backend_obj_runtime_rv64_cts_00012`,
`backend_obj_runtime_rv64_return_add_sub_chain`,
`backend_riscv_object_emission`).
