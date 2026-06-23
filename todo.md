Status: Active
Source Idea Path: ideas/open/317_rv64_select_chain_short_circuit_runtime_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize Select-Chain Residual Evidence

# Current Packet

## Just Finished

Lifecycle activation created this executor-compatible todo skeleton for Step 1
of `plan.md`.

## Suggested Next

Start Step 1 by reproving `src/00046.c`, capturing emitted RV64 assembly and
prepared BIR evidence, and identifying whether the first bad mechanism is
select-chain materialization, short-circuit control emission, value
publication, or a separate residual.

## Watchouts

- Do not reopen aggregate-local subobject stores/loads repaired by idea 314
  unless fresh evidence proves they are again the first bad mechanism.
- Do not special-case `src/00046.c`, fixed offsets, or one exact expression
  shape.
- Preserve qemu runtime proof; do not claim progress through expectation
  weakening or unsupported downgrades.

## Proof

Lifecycle-only activation. No build or tests were run.
