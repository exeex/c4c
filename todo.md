Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Existing Global Address Surfaces

# Current Packet

## Just Finished

Activation only. No implementation packet has run yet.

## Suggested Next

Start Step 1 by inspecting existing global address carriers, AArch64 selected
instruction records, printer paths, and current unsupported diagnostics.

## Watchouts

Keep global memory load/store lowering separate from address materialization.
Do not infer GOT, direct, label, or TLS policy from rendered symbol names.

## Proof

Lifecycle activation only; no build or test proof required.
