Status: Active
Source Idea: ideas/open/03_lir_to_backend_ir_refactor.md
Source Plan: plan.md

# Active Queue: LIR To Backend IR Refactor

## Queue
- [ ] Step 1: Re-establish the lowering boundary
  - Notes: Start with the narrowest behavior-preserving rename/extraction around `adapt_minimal_module()` and its callers.
  - Blockers: None.
- [ ] Step 2: Isolate LIR syntax decoding
  - Notes: Defer until the new lowering entrypoint/file ownership exists.
  - Blockers: Depends on Step 1.
- [ ] Step 3: Make backend IR more backend-native
  - Notes: Defer until decode ownership is isolated enough to expose a clean IR slice.
  - Blockers: Depends on Step 2.
- [ ] Step 4: Shift target codegen onto backend semantics
  - Notes: Defer until at least one structured operand/call slice exists in backend IR.
  - Blockers: Depends on Step 3.
- [ ] Step 5: Validate positioning for later BIR work
  - Notes: Closeout only after the active structural slice lands and backend validation is stable.
  - Blockers: Depends on prior steps.

Current Step: Step 1
Next Step: Inspect `adapt_minimal_module()` call flow and land the smallest lowering-entrypoint extraction that keeps behavior unchanged
Blockers: None
