Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Audit Existing Allocation Surfaces

# Current Packet

## Just Finished

Activation initialized the runbook from Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: audit the implemented AArch64 ABI/module/codegen record
surfaces and markdown artifacts named in `plan.md`, then update this file with
the concrete allocation-boundary conflict list before implementation edits
begin.

## Watchouts

- Do not implement a full register allocator.
- Do not store AArch64 physical-register names, assembly text, or concrete
  stack offsets as core BIR semantic truth.
- Keep GPR and FPR/SIMD register classes separate.
- Keep reserved MIR scratch registers unavailable for long-lived BIR value
  homes.
- Create a separate `ideas/open/` gap idea if missing shared BIR/prepared
  carriers block the contract.

## Proof

Lifecycle-only activation; no build proof required.
