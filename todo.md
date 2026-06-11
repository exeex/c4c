Status: Active
Source Idea Path: ideas/open/205_route6_call_use_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Route 6 Adapter Boundary

# Current Packet

## Just Finished

Step 1 selected the first Route 6 adapter boundary.

Selected call instruction: the AArch64 BIR `CallInst` currently being lowered
by `lower_scalar_call_argument_producers(...)`, identified only after
`context.bir_block->insts[instruction_index]` is confirmed to be that
`bir::CallInst`.

Selected source role: one scalar call argument source-producer materialization,
the `args[argument_index]` value used by
`materialize_scalar_call_argument_value(...)` through
`find_scalar_call_argument_source_producer_materialization(...)`.

Route 6 may read only semantic identity facts for that selected role:

- `route6_find_call_argument_source_producer(...)` status for the selected
  block, call instruction index, callee, and argument index.
- The selected argument/source relationship: `argument_value`,
  `source_value`, `source_value_name`, `source_value_id`, `source_encoding`,
  `source_kind`, and call-site ordinal fields.
- Same-block source producer identity and materialization eligibility:
  producer instruction pointer/index, producer kind, producer source value,
  and `scalar_materialization_available`.
- Existing fail-closed Route 6 statuses: missing call, wrong call, missing
  argument, missing source relationship, missing source producer,
  duplicate relationship, no match, and ABI-bound exclusion.

## Suggested Next

Execute Step 2 by adding a fail-closed adapter helper around the selected
`find_scalar_call_argument_source_producer_materialization(...)` Route 6 path.
It should validate Route 6/prepared source-id agreement for the selected scalar
argument source-producer role, then fall back to the existing prepared producer
lookup whenever Route 6 is absent, mismatched, ambiguous, ABI-bound, or missing
a materializable producer.

## Watchouts

- Prepared call-plan facts remain authoritative for the selected boundary:
  argument selection applicability, ABI destination placement, wrapper kind,
  clobbers, outgoing stack sizing, byval lanes, variadic FPR counts,
  helper/carrier protocols, value homes, move bundles, aggregate transport,
  local aggregate address publication permission, direct-global publication
  routing policy, final call records, selected machine output, and diagnostics.
- Existing coverage already exercises Route 6 BIR record/index success,
  missing/after-call producer failure, missing relationship, unnamed source,
  wrong call, no match, ABI-bound rejection, duplicate relationship rejection,
  and x86 missing-facts/source-id agreement fallback.
- Targeted Step 2 coverage should add AArch64 adapter-level proof for prepared
  source-id mismatch fallback, absent Route 6 fallback, duplicate/ambiguous
  rejection, ABI-bound rejection, missing materializable producer fallback, and
  unchanged AArch64 output for the selected scalar argument producer route.
- Do not broaden this packet into result lanes, direct-global dependency
  routing, whole call plans, helper protocol changes, expected-output rewrites,
  or unsupported-test downgrades.

## Proof

No build or test run required for this Step 1 selection-only packet.

Future Step 2 proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

`test_after.log` was not produced for this selection-only packet.
