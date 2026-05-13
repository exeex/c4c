Status: Active
Source Idea Path: ideas/open/206_prepared_memory_volatility_address_space_carrier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Existing BIR And Prepared Memory Surfaces

# Current Packet

## Just Finished

Activation created the active runbook and initialized this executor-compatible
scratchpad for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: audit BIR memory facts, prepared memory structs,
builder sites, and test fixture options before making code changes.

## Watchouts

- Do not implement target-local volatility or address-space handling as a
  substitute for the shared prepared carrier.
- Do not parse printed BIR, rendered names, debug dumps, or testcase-specific
  strings to recover memory semantics.
- Keep target MIR memory lowering, assembler/object/linker work, atomics,
  inline asm, and alias analysis out of this packet.

## Proof

Lifecycle activation only. No build proof required for this packet.
