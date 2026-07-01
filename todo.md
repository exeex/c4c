Status: Active
Source Idea Path: ideas/open/505_select_publication_stack_home_intent_support.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Supported Or Fail-Closed Stack-Home Intent

# Current Packet

## Just Finished

Step 2 added narrow select-publication stack-home intent support inside
`EdgePublicationMoveIntent` construction without implementing a new final RV64
select-publication materialization consumer.

The adapter now initializes `source_type` and `destination_type` from the
matching `PreparedEdgePublication`, represents pointer/XLEN concrete stack
sources to GPR destinations with `ld` instruction text and concrete
`source_stack_*`/destination-register fields, and represents direct GPR sources
to 1/2/4-byte concrete stack destinations with `sb`/`sh`/`sw` instruction text
and concrete `destination_stack_*` fields.

Focused coverage in `backend_riscv_object_emission` proves the
`src/builtin-constant.c` representative shape is represented as an available
pointer stack-source intent while final RV64 select-publication object
admission remains fail-closed, the `src/pr58726.c` representative shape is
represented as an available direct-register-to-i16-stack intent using
module-local ids, final RV64 select-publication object admission also remains
fail-closed for that i16 stack-destination representative, and a large
stack-destination offset still fails closed as `UnsupportedDestinationHome`.

The repair also narrowed the pre-existing stack-join final admission helper to
the older 4-byte stack-destination route so the new 1/2-byte intent evidence
does not accidentally become final RV64 materialization.

Durable evidence was written under
`build/agent_state/505_step2_stack_home_intent_support/`.

## Suggested Next

Ask the plan owner to decide residual lifecycle disposition for idea 505:
either close/extend this intent-support idea or activate a focused final RV64
select-publication consumer idea that consumes the now-structured stack-home
intent fields. Keep `src/pr37924.c` routed to idea 506, not idea 505.

## Watchouts

- Do not treat the new pointer stack-source intent as final RV64 object
  admission; `prepared_select_publication_move_is_rv64_object_admitted` still
  rejects source-stack select-publication intent.
- Do not add stack-to-stack select-publication support without an explicit
  scratch/interleaving policy.
- Do not fold generic immediate materialization from `src/pr37924.c` into this
  select-publication intent work.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Preserve fail-closed behavior for missing edge/destination identity, missing
  concrete stack offset/size, non-GPR destination placement, unsupported stack
  widths, stack-to-stack select-publication without an explicit scratch policy,
  and large offsets without an explicit address materialization policy.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed after the acceptance test repair. `test_after.log` reports
`100% tests passed, 0 tests failed out of 328`, followed by a clean
`git diff --check`.
