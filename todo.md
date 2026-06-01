Status: Active
Source Idea Path: ideas/open/70_aarch64_memory_prepared_address_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Memory Authority Duplication

# Current Packet

## Just Finished

Activation only. No executor packet has completed yet.

## Suggested Next

Start `plan.md` Step 1 by mapping duplicated memory authority decisions in
`src/backend/mir/aarch64/codegen/memory.cpp` against the existing prepared
addressing, value-home, storage, frame-offset, aggregate stack-source, and typed
stack-source facts.

## Watchouts

Do not classify AArch64 offset legality, scratch-register selection, memory
opcode selection, address spelling, or va_list field addressing as shared
authority. Those remain target-local policy unless a later source idea says
otherwise.

## Proof

Lifecycle activation only; no build proof required yet.
