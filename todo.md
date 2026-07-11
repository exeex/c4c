Status: Active
Source Idea Path: ideas/open/689_memory_address_provenance_import_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Memory Import Coupling

# Current Packet

## Just Finished

Activation created the active runbook for Step 1 of
`ideas/open/689_memory_address_provenance_import_cleanup.md`.

## Suggested Next

Execute Step 1: inspect the memory/address provenance import surfaces, choose
one first behavior-preserving adapter-local narrowing target, and record the
selected packet and proof command here before implementation.

## Watchouts

- Keep the work adapter-local to `src/backend/bir/lir_to_bir/memory/`,
  `memory_types.hpp`, `memory_helpers.hpp`, and only necessary supporting
  declarations.
- Do not edit prepared frame/storage policy, target addressing legality, MIR
  memory emission, tests, expectations, unsupported markers, allowlists,
  runtime behavior, or harness policy.
- Do not move public BIR Route 3 authority into private lowering.

## Proof

No validation run for lifecycle-only activation.
