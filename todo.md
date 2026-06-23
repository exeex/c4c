Status: Active
Source Idea Path: ideas/open/321_rv64_i16_local_array_select_store_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize I16 Local-Array Select/Store Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00143.c`, confirming the stack-homed fused
compare loop branch has already emitted, and classifying the first i16
local-array select/store publication failure in the loop body.

## Watchouts

- Do not special-case `src/00143.c`, `.Lmain_block_1`, `.Lmain_block_2`,
  `%t9.store0`, fixed source spelling, or observed stack offsets.
- Do not claim progress by widening the focused fixture from i16/halfword
  storage to int storage unless that is explicitly recorded as
  reclassification rather than repair.
- Do not fold stack-homed fused compare control flow, nested select-chain
  store-source publication, aggregate/byval, function-pointer, external-call,
  or broad switch/control-flow work into this route.

## Proof

Lifecycle-only activation. No build or tests were run.
