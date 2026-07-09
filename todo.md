Status: Active
Source Idea Path: ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate prepared destination authority production

# Current Packet

## Just Finished

Completed Step 2: located the prepared destination authority production
surface for aggregate/byval outgoing stack argument facts.

Artifact:
`build/agent_state/624_step2_destination_authority_surface/README.md`

Concrete producer target:

- `src/backend/prealloc/call_plans.cpp`
- `plan_call_argument_destination(...)`
- local `CallArgumentDestinationPlan`
- published fields on `PreparedCallArgumentPlan`:
  `destination_stack_offset_bytes` and `destination_stack_size_bytes`
- call-level coverage carrier:
  `PreparedCallPlan::outgoing_stack_argument_area`, computed by
  `compute_outgoing_stack_argument_area(...)`
- aggregate transport mirror:
  `PreparedAggregateTransportPlan::destination_stack_offset_bytes` and
  `destination_stack_size_bytes`

Existing upstream authority:

- `src/backend/prealloc/regalloc/call_return_abi.cpp`
- `regalloc_detail::call_arg_destination_stack_offset_bytes(...)`
- `prepared_call_stack_argument_size_bytes(...)`

Current finding:

- Scalar stack args already publish prepared destination facts from before-call
  ABI bindings; `src/pr69447.c` shows `dest_stack_offset=0`,
  `dest_stack_size=8`, and `outgoing_stack_argument_area=8`.
- Aggregate/byval stack-copy args already have callsite, argument identity,
  source identity, source selection, and aggregate transport payload/chunk
  facts, but they do not publish destination offset/size when no register
  binding exists.
- The smallest producer route is to have `plan_call_argument_destination(...)`
  publish the existing helper-derived stack destination offset plus size for
  complete stack-destination aggregate/byval args, so downstream consumers read
  prepared authority instead of inferring ABI layout.

Facts to publish:

- destination offset
- destination size
- payload/source identity
- argument identity
- callsite association
- outgoing area/coverage

Expected proof rows after implementation:

- `src/20000808-1.c`
- odd `src/931004-*` rows (`1,3,5,7,9,11,13`)
- `src/931031-1.c`
- `src/950607-2.c`
- scalar guard `src/pr69447.c`

## Suggested Next

Execute Step 3 from `plan.md`: publish prepared outgoing destination facts in
`plan_call_argument_destination(...)` for aggregate/byval outgoing stack-copy
arguments using the existing stack-destination helper and prepared size helper.
Then prove with `--dump-prepared-bir` rows that aggregate args expose
`dest_stack_offset`, `dest_stack_size`, transport destination facts, and
`outgoing_stack_argument_area` before RV64 consumes them.

## Watchouts

- Do not infer aggregate destination offsets inside RV64 from ABI index, final
  assembly layout, source filename, or testcase shape. RV64 should consume
  prepared facts only.
- Missing destination offset, destination size, outgoing area, or incomplete
  aggregate transport coverage must remain fail-closed.
- Keep `src/pr69447.c` as a scalar guard for existing stack-destination facts,
  not as aggregate/byval proof.
- Keep even `src/931004-*` rows out of the first implementation proof until
  their semantic local-memory blocker is separately owned.
- Do not broaden into variadic/library policy, runtime mismatch, local/global
  producers, stack-frame consumers, expectations, unsupported markers,
  allowlists, timeouts, or accounting changes.

## Proof

Proof command:
`cmake --build build --target c4cll > test_after.log 2>&1`

Result: passed. `test_after.log` contains `ninja: no work to do.` after the
target check.
