Status: Active
Source Idea Path: ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Define the checked overflow aggregate carrier contract

# Current Packet

## Just Finished

- None. Blocker plan activated; executor packet has not started.

## Suggested Next

- Establish the smallest native contract for the AMD64 overflow-area aggregate
  `va_arg` memcpy carrier, then choose only the matching implementation packet.

## Watchouts

- This blocker owns only the derived AMD64 overflow aggregate carrier. Do not
  turn it into generic aggregate/vector, ABI, or memory authority work, and do
  not use text recovery.

## Proof

- Before handoff: fresh build plus focused AMD64 aggregate `va_arg` positive and
  malformed-carrier proof. Preserve 753's accepted 3037/3037 full-baseline
  reference; the supervisor selects any broader proof.
