Status: Active
Source Idea Path: ideas/open/348_vrm_regalloc_mir_and_rv64_assembler_bridge.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Target-Neutral Spill/Reload Pseudo Contract

# Current Packet

## Just Finished

Completed Step 3: Target-Neutral Spill/Reload Pseudo Contract. Prepared
spill/reload pseudo records now carry value id, source value name when known,
register bank/class, base register name, contiguous width, occupied register
names, slot id/offset, register placement when already published, and a
structured spill-slot placement derived from slot id/offset when needed.
Focused coverage proves grouped vector, general, and float spill/reload records
remain structured prepared data and that the prepared printer exposes those
fields without adding target assembly load/store text.

## Suggested Next

Delegate Step 4: connect the target-neutral spill/reload pseudo contract to the
next MIR/RV64 handoff point without rendering final target load/store assembly
in the prepared layer.

## Watchouts

- Do not add any VRM function-call ABI behavior.
- Do not downgrade supported-path expectations to unsupported.
- Float grouped spill authority currently publishes source value, bank/class,
  width, occupied registers, and structured spill-slot placement, but not a
  register placement unless the upstream allocator already provides one.
- Spill/reload records intentionally remain target-neutral pseudo data; target
  load/store or custom vector memory instruction rendering belongs to a later
  lowering packet.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepared_printer|backend_prepare_frame_stack_call_contract)$'`

Proof log: `test_after.log`.
