Status: Active
Source Idea Path: ideas/open/690_call_abi_import_boundary_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish Call Import Evidence And First Packet Boundary

# Current Packet

## Just Finished

Activation reset for `ideas/open/690_call_abi_import_boundary_cleanup.md`.

## Suggested Next

Execute Step 1 from `plan.md`: read the call ABI import source idea and
handoff docs, inspect `call_abi.cpp`, `calling.cpp`, and any relevant
adapter-private declarations, then choose one narrow behavior-preserving first
packet and record its proof command here.

## Watchouts

- Keep this idea confined to call ABI import cleanup.
- Do not touch prepared call plans, physical register placement, outgoing
  stack layout, aggregate lane transport, wrappers, helper protocols, MIR
  consumers, target emission, tests, expectations, unsupported markers,
  allowlists, runtime behavior, or harness policy.
- If the first packet needs downstream prepared/target ownership changes,
  stop and split that work into a separate idea instead of expanding this
  plan.

## Proof

Not run. Lifecycle activation only.
