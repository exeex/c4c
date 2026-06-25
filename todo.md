Status: Active
Source Idea Path: ideas/open/352_full_rv64_assembly_object_disassembly_roundtrip.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Declare RV64I Corpus And Canonical Contract

# Current Packet

## Just Finished

Activation only. No executor packet has completed for Step 1 yet.

## Suggested Next

Execute Step 1 from `plan.md`: declare the RV64I plus c4c EV64 corpus and add
the initial CTest-compatible roundtrip scaffolding.

## Watchouts

- Do not treat the existing `.insn.d`, `li`, and `ret` subset as completion for
  this idea.
- Keep unsupported extensions and directives fail-closed.
- Do not use external assembler or objdump output as the source of truth.

## Proof

Lifecycle activation only; no build or CTest proof required for this packet.
