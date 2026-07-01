Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Stack-To-Stack Move Evidence

# Current Packet

## Just Finished

Activated Step 1 from `plan.md` for idea 513.

## Suggested Next

Rebuild or inspect the representative artifacts for `src/20010518-1.c`,
`src/pr27073.c`, and `src/pr69447.c`; record the prepared move bundle, source
home, destination home, width, alignment, and current
`unsupported_move_bundle_target_shape` site in this file.

## Watchouts

Keep this route in RV64 consumption only when prepared move/source/destination
facts are explicit and coherent. If the producer authority is missing, stop and
split that producer gap instead of inferring stack locations in RV64.

## Proof

Activation only; no code validation required.
