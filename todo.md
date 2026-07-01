Status: Active
Source Idea Path: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Publish Available Interval Facts From Stored Stream

# Current Packet

## Just Finished

Lifecycle switched from closed idea 497 back to idea 494 after the 497 Step 5
disposition said no lower endpoint/effect blocker remains.

## Suggested Next

Execute Step 5 from `plan.md`: publish available interval facts only from a
matching production ordered effect-source stream consumed through the endpoint
bridge, while preserving the existing fail-closed status surface.

## Watchouts

- Do not infer availability from selected path availability, synthetic bridge
  flags, caller-supplied effect vectors, final homes, target behavior, or
  `lir_producer_instruction_index` alone.
- Keep downstream proof-fact population, checker input population, idea 490
  certification, packaging, scalar loads, and RV64/MIR lowering out of this
  plan.

## Proof

Lifecycle switch proof:

```sh
git diff --check
```

Result: passed.
