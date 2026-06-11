Status: Active
Source Idea Path: ideas/open/197_return_chain_import_and_naming_clarification.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Neutral Facts and Target Policy

# Current Packet

## Just Finished

Step 2 classification is complete. Return-chain should be imported as its own
Route 8 / return-chain owner-schema line with target-neutral identity facts
kept separate from target-local AArch64 lowering policy and historical prepared
oracle surfaces.

| Surface | Classification | Current import rule |
| --- | --- | --- |
| Function/block scope | Target-neutral return-chain identity. The record is function-local and block-local. | Route 8 may key by function plus BIR block or stable block identity; it must not stitch chains across blocks. |
| Instruction position/reference | Target-neutral route key. | The key may use the current instruction index or an equivalent stable instruction reference, and must be rebuilt or invalidated after relevant BIR order changes. |
| Chain value | Target-neutral route key and semantic subject. | The value must be the named scalar binary result at the keyed instruction, with stable BIR/prepared name identity. |
| Accepted chain shape | Target-neutral route acceptance. | Same-block scalar binary chains ending at a named return terminator value are accepted only for the proven scalar publication opcodes from the decision doc. |
| Terminal return value | Target-neutral route answer. | Route 8 owns the named value consumed by the return terminator when the walked chain reaches it. |
| Immediate next operand | Target-neutral optional route answer. | Route 8 owns the first next binary's other operand only when that operand has a stable name; otherwise the next-operand answer is absent. |
| Rejected, ambiguous, and conflicting cases | Target-neutral fail-closed route status. | Unsupported opcodes, unnamed values, non-return terminators, broken walks, cross-block relationships, missing keys, and duplicate conflicting records produce no usable semantic answer rather than target-selected fallback data. |
| ABI return moves and destination bank | AArch64/target-local policy. | AArch64 chooses before-return ABI moves and destination banks after Route 8 identifies the terminal value. |
| Value homes and prepared-name translation | AArch64/prepared materialization policy. | AArch64 may translate a Route 8 BIR identity through prepared names/value homes to reuse existing target lowering, but this is not return-chain identity ownership. |
| Register conversion/parsing and scalar register view | AArch64/target-local policy. | Prepared-register names, AArch64 register parsing/conversion, expected scalar views, and retargeted `RegisterOperand` metadata stay in lowering. |
| Alias checks and scratch selection | AArch64/target-local policy. | Next-operand alias detection and reserved GP scratch choice remain target policy after the route answers the identity question. |
| Final ALU records and emission order | AArch64/target-local policy. | Scalar ALU record construction, before-return placement, and final instruction order are not Route 8 facts. |
| Historical prepared terminal/next helpers | Historical migration oracle. | Ideas 176-179 used public prepared helpers as oracle and migration fallback surfaces while Route 8 was introduced and AArch64 consumers were migrated. |
| Current public prepared return-chain helpers | Absent after idea 180 API contraction. | `PreparedReturnChainLookups`, `find_prepared_return_chain_terminal_value`, `find_prepared_return_chain_next_operand_value`, `prepared_return_chain_value_key`, and the terminal/next map names have no current `src`/`tests` matches. Future docs should not list them as retained public blockers. |
| Current prepared-owned residual surface | Diagnostic/materialization support only. | AArch64 still uses prepared value-location/name data to materialize Route 8 identities into target homes/registers, but it must not consult prepared return-chain fallback facts. |

Route 8 remains distinct from Route 1 producer identity and Route 7 comparison
provenance. Its semantic answer is not "who produced this value" or "which
comparison/branch provenance applies"; it is the same-block relation from a
scalar binary chain value to the terminal return value and optional immediate
next operand. It also must not be described as predecessor rescan, name
matching, or a generic route-index facade.

## Suggested Next

Execute Step 3: define the durable name and citation rules for future docs.
Suggested accepted spelling: `Route 8 return-chain owner/schema line`, with
`return-chain owner/schema line` acceptable when the route number is not the
main point. Step 3 should state how `PreparedFunctionLookups` and
`PreparedBirModule` readiness may cite the completed line without implying
Routes 1 or 7, retained public prepared helpers, predecessor rescans, name
matching, or broad aggregate retirement progress.

## Watchouts

- Keep the public prepared helper status time-aware: closed ideas 176-179 and
  the decision doc describe a historical public oracle surface; closed idea 180
  removed that public API, and current `src`/`tests` searches confirm absence.
- Do not let AArch64 target policy leak into the route identity. Homes,
  registers, ABI moves, alias handling, scratch selection, ALU records, and
  emission order are downstream lowering policy only.
- Do not use Route 8 as evidence that `PreparedFunctionLookups` or
  `PreparedBirModule` is broadly ready for retirement. It is one completed
  return-chain line, not a replacement for field-by-field lookup ownership.
- Do not reopen implementation, tests, expectations, unsupported markers, or
  closed idea files in this docs/lifecycle-only plan.

## Proof

Docs/lifecycle-only classification. Read the active plan/source idea, the
return-chain decision doc, closed ideas 176-180, current Route 8 declarations
in `src/backend/bir/bir.hpp`, and the AArch64 Route 8 consumer boundary in
`src/backend/mir/aarch64/codegen/alu.cpp`. Ran targeted `rg` scans for
return-chain, Route 8, and removed prepared helper names under `src`, `tests`,
`docs/bir_prealloc_fusion`, `ideas/open`, and `ideas/closed`; the removed
public prepared helper names have no current `src`/`tests` matches. No
build/test subset was required and no `test_after.log` was generated.
