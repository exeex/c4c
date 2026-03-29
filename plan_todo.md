# Plan Todo

Status: Active
Source Idea: ideas/open/33_backend_x86_external_call_object_plan.md
Source Plan: plan.md

## Current Active Item

- [ ] Step 1: identify or add one bounded x86 backend fixture that emits an
      undefined helper call so the external assembler produces the staged
      `R_X86_64_PLT32` object contract.

## Ordered Checklist

- [ ] Step 1: Capture the external-call reference contract.
- [ ] Step 2: Tighten focused validation.
- [ ] Step 3: Implement the minimal assembler slice.
- [ ] Step 4: Prove end-to-end behavior and guard regressions.

## Next Intended Slice

- inspect the current x86 backend fixture inventory and select the narrowest
  seam for an undefined `helper_ext` call
- capture external assembler relocation metadata for that fixture before adding
  validation
- keep idea 32 parked as the local-call contract discovery record rather than
  reusing it for relocation-bearing work

## Blockers

- None yet beyond identifying the narrowest backend-owned undefined-helper
  fixture. Preserve the prior discovery that `call_helper.c` remains a
  relocation-free local-call case and should not be reused for this plan.

## Resume Notes

- This active plan intentionally supersedes idea 32 after its Step 1 discovery
  showed that `call_helper.c` is not a relocation-bearing contract.
- Keep the implementation bounded to one relocation-bearing external-call
  helper object path. Split again if broader relocation or linker work becomes
  necessary.
- The staged x86 linker contract to preserve remains:
  `R_X86_64_PLT32` at `.text` offset `1` with addend `-4` targeting
  `helper_ext`.
