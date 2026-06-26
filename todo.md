Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Current Instruction-Fragment Residuals

# Current Packet

## Just Finished

Activated `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
after closed idea 407 routed the remaining `src/divmod-1.c` generic
`unsupported_instruction_fragment` residual back to this parent route.

## Suggested Next

Delegate Step 1 to an executor: reclassify the current
`unsupported_instruction_fragment` representatives, starting with
`src/divmod-1.c` and `src/20000223-1.c`, and record the first concrete
instruction-fragment family plus proof command here.

## Watchouts

- Do not reopen 407 unless fresh prepared dumps show the old no-bank
  same-module `i16` call-argument shape again.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Route producer-fact gaps to their owners instead of patching them under 395.

## Proof

Lifecycle activation only. No build or test proof was run for this
plan-owner-only reset.
