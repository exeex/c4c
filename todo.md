Status: Active
Source Idea Path: ideas/open/212_bir_mir_allocation_contract.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Commit The Allocation Contract

# Current Packet

## Just Finished

Step 2 of `plan.md` added
`src/backend/mir/aarch64/ALLOCATION_CONTRACT.md` as the canonical
BIR/prepared-to-target-MIR allocation-boundary contract.

The contract defines finite AAPCS64 GPR and FPR/SIMD register budgets, separate
long-lived, argument/return, caller-saved temp, reserved MIR scratch, and
special/forbidden pools, physical allocation result shapes, structured
spill-slot ids, future virtual-register placeholders, call-clobber and
callee-save obligations, reload/store pseudo plan placement, and portable
x86/x86-64 correctness carriers for fixed operands, flags, subregister aliases,
width effects, ABI variants, and ABI stack resources.

## Suggested Next

Execute Step 3: align implemented AArch64 ABI, module, and codegen record
surfaces so they explicitly consume, expose, or avoid allocation ownership
without introducing an allocator, concrete frame offsets, reload/store
instruction emission, or machine-node opcode expansion.

## Watchouts

- Step 3 should keep `abi/` helpers as target register facts and conversion
  utilities, not as allocation algorithms.
- Module records currently expose prepared register names and concrete stack
  offsets; Step 3 should document or narrow those as prepared snapshots, not
  allocation authority.
- Record builders should continue to fail closed for unsupported stack/spill
  homes until a structured allocation-result or reload/store pseudo plan is
  represented.
- Keep reserved MIR scratch registers unavailable for long-lived BIR value
  homes.

## Proof

`git diff --check` passed for this Step 2 markdown contract slice. Proof
output was preserved in `test_after.log`; the file is empty because the
command produced no diagnostics.
