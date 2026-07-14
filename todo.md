# Current Packet

Status: Active
Source Idea Path: ideas/open/774_raw_bir_gep_function_label_address_base.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Specify the structured Raw-BIR GEP-base variant

## Just Finished

- Lifecycle switch: 773 Step 1 remains accepted in `a4415f99c`; its Step 2 is
  paused because the Raw-BIR `GlobalObjectId`-only GEP-base schema/builder
  boundary is outside 773's source scope.

## Suggested Next

- Execute Step 1: define the bounded structured Raw-BIR GEP-base variant,
  retain the global-array contract, and add focused structural/malformed
  coverage. Do not perform 773 printer work or 772 forwarding work.

## Watchouts

- The function-owned direct label address must remain typed authority. Do not
  recover it from text, manufacture SSA/globals, or admit generic ValueIds.

## Proof

- No blocker implementation proof yet. The accepted outgoing evidence is
  `a4415f99c` with a fresh build and
  `^backend_lir_to_bir_interface$` pass for 773 Step 1 only.
