Status: Active
Source Idea Path: ideas/open/494_dynamic_local_array_lir_producer_interval_effect_classifier.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Interval Effect Inputs

# Current Packet

## Just Finished

Step 1 audit completed for interval-effect inputs.

Findings:

- Dynamic-index identity exists as a bounded input: local-array element paths
  publish one dynamic `LocalArrayIndexRecord` for the representative cases, and
  the selected proof-edge collector only accepts a fused compare when the proof
  operand is the same named value and type as that dynamic index.
- Populated selected proof-edge records exist and are keyed by exact
  `lir_producer_lookup_key`, function spelling, proof block, selected outcome,
  selected/non-selected successor labels, bound contribution, path coverage,
  dominance/guard booleans, and LIR producer coordinate/status.
- Interval start is bounded for available cross-block records at the selected
  successor edge/entry. Same-block proof/producer records remain deliberately
  fail-closed as `missing_same_block_ordering`.
- Interval end is not truthfully bounded today. The only producer endpoint on
  the local-array path/selected proof-edge record is
  `lir_producer_instruction_index`, and that is a LIR producer-site coordinate,
  not a prepared/BIR instruction index. No current prepared record maps the
  `lir_producer_lookup_key` or LIR producer coordinate to an ordered
  prepared/BIR endpoint for "before the address derivation."
- Effect/no-clobber input surfaces exist but are not yet enough to publish an
  available interval fact:
  assignments/redefinitions are represented by BIR SSA named values and value
  homes; phi/alias and selected-edge transfers are represented by join
  transfers, edge publications, and parallel-copy bundles; calls/helpers expose
  call plans, clobbered registers, preserved values, and call-boundary effect
  plans; inline asm exposes carriers with side-effect and clobber metadata;
  publications expose source/destination homes and memory-access facts; move
  bundles and parallel copies expose ordered prepared move data. These surfaces
  can feed fail-closed statuses, but they cannot be sliced to the dynamic-array
  producer endpoint without the missing endpoint bridge.

Conclusion: a bounded classifier can be designed to fail closed, but an
available same-value/no-clobber classifier is blocked lower down by the missing
LIR-producer-to-prepared/BIR endpoint/order bridge. The exact lower blocker is
to publish or derive an authoritative prepared/BIR event coordinate for the
local-array `lir_producer_lookup_key` address-derivation site without treating
`lir_producer_instruction_index` as that coordinate.

## Suggested Next

Route Step 2 around a fail-closed interval classifier contract, or first assign
a lower bridge packet that publishes the prepared/BIR producer endpoint for a
local-array `lir_producer_lookup_key`. Do not implement available interval facts
until that bridge exists.

## Watchouts

- Do not infer same-value/no-clobber from selected path records alone.
- Do not populate idea 489 proof facts or idea 486 checker inputs directly.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate.
- Same-block proof/producer cases remain fail-closed unless a later truthful
  ordering bridge exists.
- The missing endpoint bridge also affects cross-block available records when
  the selected successor block is the producer block: effects before the
  address derivation cannot be separated from later block effects.
- Existing untracked `review/*.md` files are transient and must remain
  untouched.

## Proof

Step 1 audit proof:

```sh
git diff --check
```

Result: passed. This delegated classification-only proof does not write
`test_after.log`; the packet also marked canonical log files out of scope.
