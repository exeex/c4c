Status: Active
Source Idea Path: ideas/open/407_prepared_i16_same_module_call_arg_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate The Caller-Side Publication Gap

# Current Packet

## Just Finished

Lifecycle review split the `src/divmod-1.c` residual out of 395. The active
runbook now targets producer-side same-module `i16` call-argument ABI
publication.

## Suggested Next

Delegate Step 1 to locate the caller-side publication gap. The executor should
compare the prepared `i8` and `i16` same-module call artifacts, identify the
producer path that leaves the `i16` argument as `value_bank=none`,
`source_encoding=frame_slot`, `dest_bank=none`, and report the first repair
target before implementation.

## Watchouts

- Do not patch RV64 `object_emission.cpp` to infer the missing scalar argument
  destination.
- Keep closed 403 formal ABI publication separate unless fresh facts show a
  regression in incoming formal handling.
- Preserve `test_after.log` as the canonical executor proof log.

## Proof

Lifecycle-only review. No build or broad validation was run.
