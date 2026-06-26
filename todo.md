Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm Current Frame-Slot I16 Destination Gap

# Current Packet

## Just Finished

Lifecycle repaired after active 395 Step 1 showed the next implementation
target must not be RV64 opcode lowering. Fresh reclassification for
`src/divmod-1.c` still shows same-module `i16` frame-slot call arguments with
`value_bank=gpr` but `dest_bank=none` and `reason=call_arg_stack_to_stack`.
`src/20000223-1.c` passes and does not share this blocker.

Reopened `ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md`
because this live producer-side destination publication gap still matches its
source intent.

## Suggested Next

Delegate Step 1 to an executor: identify the producer helper/function family
that should publish complete GPR call-argument destination facts for
same-module `i16` frame-slot sources, then record the exact proof command here.

## Watchouts

- Do not implement 395 RV64 opcode lowering while this producer blocker
  remains.
- Do not infer scalar call-argument ABI policy inside RV64 object emission.
- The live incomplete shape is partially repaired compared with the oldest
  407 notes: `value_bank=gpr` is present, but `dest_bank=none` and
  `call_arg_stack_to_stack` remain blocking.

## Proof

Lifecycle repair only. Evidence came from the active 395 Step 1 `todo.md`
record and supervisor clean-rebuild confirmation; no new build or test was run
by the plan owner.
