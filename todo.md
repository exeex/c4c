# Current Packet

Status: Active
Source Idea Path: ideas/open/827_lir_direct_scalar_binary_lhs_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Identify the actual failing operation and reproduce its type relation

## Just Finished

- Step 1 trace complete and committed as `26c000c01`: the producer selects a native definition by LHS value, operation type, and DirectScalar ABI, while the verifier's missing-authority relation can select the same LHS value and ABI without the type predicate. This identifies a value/ABI match with a definition-type versus operation-type mismatch as the unproven seam.

## Suggested Next

- Step 2: run the full parent route diagnostically to identify its first exact failing lowered `LirBinOp`, then create a minimal standalone reproduction showing the same native DirectScalar value/ABI match and definition-type / operation-type mismatch. Only after that evidence may a producer repair be designed.

## Watchouts

- The dirty producer/test proposal is unaccepted: adding only a current-owner predicate does not address the discovered type mismatch, and the `ull x + 1` shape is not evidence of the full-route abort. Do not modify, accept, or commit it in this packet. Preserve the existing verifier contract and keep selector, Raw-BIR/importer, RHS, return, pointer, and generic routes out.

## Proof

- This is a trace/reproduction packet: preserve the existing dirty worktree and do not create or roll forward `test_after.log`. Record the diagnostic full route and standalone reproduction commands/results in the executor update; no acceptance proof or commit is authorized.
