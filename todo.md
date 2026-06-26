Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Current Instruction-Fragment Residuals

# Current Packet

## Just Finished

Activated `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`
after corrected close of
`ideas/closed/407_prepared_i16_same_module_call_arg_abi_publication.md`.

Closed 407 records that `src/divmod-1.c` frame-slot same-module `i16`
producer facts are complete and the remaining failure is generic
`unsupported_instruction_fragment`, routed back to 395.

## Suggested Next

Delegate Step 1 to an executor: reclassify the current
`unsupported_instruction_fragment` representatives, starting with
`src/divmod-1.c` and `src/20000223-1.c`, confirm the corrected 407 producer
facts remain complete, and record the first concrete instruction-fragment
family plus proof command here.

## Watchouts

- Do not reopen 407 unless fresh prepared dumps again show the old
  same-module `i16` producer regression:
  `source_encoding=frame_slot ... dest_bank=none`,
  `call_arg_stack_to_stack`, or `placement=none:call_argument`.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- Route producer-fact gaps to their owners instead of patching them under 395.

## Proof

Lifecycle activation only. No build or test proof was run for this
plan-owner-only reset.
