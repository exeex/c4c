# Idea 494 Step 1 - Interval Effect Input Audit

## Outcome

Step 1 found a lower blocker for publishing available dynamic-index
same-value/no-clobber interval facts.

A bounded fail-closed classifier contract can be written, but the available
classifier cannot be built truthfully with the current inputs because the
local-array producer endpoint is only recorded as a LIR producer coordinate.
There is no current authoritative bridge from `lir_producer_lookup_key` or
`lir_producer_instruction_index` to the prepared/BIR event order for the address
derivation site.

## Inputs Found

- Dynamic-index identity:
  `LocalArrayElementPathRecord::indices` carries the dynamic named value.
  The selected proof-edge collector requires the proof compare operand to match
  that dynamic index by name and type.
- Selected proof-edge records:
  `local_array_selected_proof_edge_paths` records carry status, exact
  `lir_producer_lookup_key`, LIR producer function/block/instruction
  coordinate, proof block, proof lhs/rhs/predicate/type, selected outcome,
  selected/non-selected successor labels, bound contribution, path coverage,
  and dominance/guard booleans.
- Interval start:
  available cross-block records can start at the selected successor edge/entry.
  same-block proof/producer records already fail closed as
  `missing_same_block_ordering`.
- Effect surfaces:
  BIR SSA values cover ordinary assignment identity; join transfers, edge
  publications, and parallel-copy bundles cover phi/alias edges; call plans,
  clobbered registers, preserved values, and call-boundary effects cover calls
  and helpers; inline asm carriers cover side effects and clobbers; publication
  plans expose source/destination homes and memory-access facts; prepared move
  bundles and parallel-copy records expose move effects.

## Blocker

Interval end is not bounded. `lir_producer_instruction_index` must remain a LIR
producer-site coordinate. Treating it as a prepared/BIR instruction index would
be coordinate confusion. Without a real bridge, the classifier cannot decide
which prepared/BIR effects in the producer block occur before the dynamic
local-array address derivation and which occur after it.

This blocks even cross-block selected-proof records when the selected successor
block is also the LIR producer block.

## Disposition

Next work should either define a fail-closed classifier contract that reports
the missing endpoint bridge explicitly, or add a lower-level bridge that
publishes the prepared/BIR address-derivation endpoint keyed by
`lir_producer_lookup_key`.
