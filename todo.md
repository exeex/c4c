# Current Packet

Status: Active
Source Idea Path: ideas/open/727_common_prepared_return_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce and attach typed return-chain authority

## Just Finished

- Completed Plan Step 1: specified the common return-chain contract from the
  fact-by-fact AArch64 reconstruction inventory below. No implementation or
  test file was edited.

### Fact inventory and earliest common owner

| Reconstructed fact / check | Identity that must survive | Earliest prepared producer seam |
| --- | --- | --- |
| Query start is one instruction result in one prepared function/block, with a scalar `bir::TypeKind` and prepared result home | function name, block label/index, instruction index, result `ValueNameId` + `PreparedValueId`, type | `make_prepared_object_function_traversal`, after extending its common inputs with prepared names/function lookups; this is the first seam where BIR instruction events and prepared locations meet |
| Required authorities exist and describe the same function/block | prepared names, control-flow block, BIR block, value locations, function lookups | traversal construction; missing input is `Absent`, disagreement is `Inconsistent` |
| Each link uses the unique `BeforeInstruction` bundle at `(block, current instruction + 1)` | bundle proof attribution/function/phase/block/instruction identities | `make_prepared_move_bundle_lookups` in `PreparedFunctionLookups`; traversal already emits the corresponding before-instruction event and lookup evidence |
| Exactly one move carries the current value and it is `Value -> Value` with a nonzero destination id | move authority/op, from/to value ids, destination kind | value-location move-bundle production; common relation producer authenticates uniqueness and supported move semantics rather than exposing a raw lookup |
| The moved-to id resolves uniquely to a complete named prepared home | destination `PreparedValueId`, `ValueNameId`, home kind and storage payload | `make_prepared_value_home_lookups`; traversal value-home classification supplies missing/ambiguous/conflicting/unsupported/incomplete distinctions |
| The moved-to name has one same-block scalar producer before the next boundary | block label, producer value name/kind, BIR instruction pointer/index | `make_prepared_edge_publication_source_producer_lookups` within `make_prepared_function_lookups`; `find_prepared_same_block_scalar_producer` is the existing common query |
| Producer is the immediately following instruction and is a binary scalar publication | producer index equals prior index + 1; producer kind `Binary`; result type | common return-chain production at traversal construction; adjacency is semantic structure, while the target must retain opcode/register policy |
| Binary result name equals the moved-to home name | result `ValueNameId` equals destination-home `ValueNameId` | prepared names + common producer query at traversal construction |
| Exactly one binary operand is the prior chain value | lhs/rhs resolved names; chain operand role; other operand role | common relation producer; preserve explicit `Lhs`/`Rhs` role and reject both/neither matches |
| On the first successor only, a named non-chain operand resolves to a unique complete prepared home; immediate/non-named is valid with no home | operand kind/name and optional home identity | prepared names + value-home lookup/classifier at traversal construction |
| A chain value terminates in one `BeforeReturn` ABI move for the scalar register bank | terminal value id/name/home, ABI move/bundle identity, destination bank/placement | value-location ABI-move production and `make_prepared_move_bundle_lookups`; common contract records prepared bank/placement only and does not convert to AArch64 registers |
| Evidence is fresh for each consumed move source and consistent with the attached traversal snapshot | value id, use site, selected freshness authority/proof, proof-attribution id | prepared move-source freshness production already consumed by `PreparedObjectMoveBundleConsumerClassification`; relation producer must require the same selected authority for every link and retain its identity |
| Traversal attachment corresponds to the starting `Instruction` event and same immutable prepared snapshot | event block/index/instruction plus relation start identity | `make_prepared_object_function_traversal`; attach classification/result to the instruction event, never synthesize it in a target consumer |

No required fact lacks a common prepared owner. The first common *composition*
owner is traversal construction because earlier lookup builders individually own
moves, homes, ABI bindings, or scalar producers but none sees all of them plus
the instruction event to which the authenticated relation must attach.

### Proposed narrow typed contract

- Add `PreparedObjectReturnChainStatus` and stable status-name support.
- Add `PreparedObjectReturnChainQuery` containing only common inputs:
  starting `PreparedObjectTraversalEvent*`, prepared names, value locations,
  value-home/move/source-producer lookups, move-source freshness authorities,
  starting result value (or authenticated result id/name/home), and scalar
  result type. It contains no target register view, opcode selection, or
  materialization callback.
- Add immutable `PreparedObjectReturnChainLink` records containing bundle/move
  identity, source and destination value id/name/home, producer identity and
  index, `chain_operand_role` (`Lhs` or `Rhs`), non-chain `bir::Value*`, and
  selected freshness authority identity.
- Add `PreparedObjectReturnChainRelation` containing `status`, start event and
  result identity, ordered non-empty links, terminal value home plus the
  prepared before-return ABI move/bank/placement, and optional first-successor
  non-chain operand home. `Available` requires the entire payload; every other
  status carries no consumable relation.
- Expose `classify_prepared_object_return_chain(query)` for focused proof, and
  attach its classification/relation to the starting instruction traversal
  event during `make_prepared_object_function_traversal`. Consumers read only
  the attached view. Production may use existing indexed lookups internally;
  consumers may not walk raw bundles or rebuild scalar-producer lookups.

### Valid chain shapes

1. One-link chain: starting result `v0`, unique before-instruction `v0 -> v1`,
   immediately adjacent binary `v1 = v0 op immediate`, then unique prepared
   before-return ABI move from `v1`. The first non-chain operand is non-named,
   so its home is absent by design.
2. Multi-link chain with alternating operand roles: `v0 -> v1`, adjacent
   `v1 = named_rhs op v0`, then `v1 -> v2`, adjacent `v2 = v1 op named_rhs2`,
   followed by the unique ABI-return move from `v2`. The relation records the
   first named non-chain operand home, both link roles, and every move/producer
   identity.

### Fail-closed negative-state matrix

| Outcome | Precise conditions (first failing classification wins) |
| --- | --- |
| `Absent` | missing start event/instruction/result; missing names, BIR block, control-flow block, value locations, required lookup family, link bundle/move, destination home, producer, terminal ABI move, or required named first-other-operand home |
| `Stale` | selected move-source freshness is missing/invalid/unsupported for a link, proof attribution no longer matches the attached bundle/snapshot, or producer/home evidence is from a different prepared snapshot |
| `Ambiguous` | duplicate positional bundle; multiple matching chain moves; ambiguous destination/operand/terminal home; multiple scalar producers; multiple ABI-return moves/bindings; or ambiguous freshness authority |
| `Inconsistent` | function/block/index identities disagree; move source is not current value; destination id/name/home disagree; producer result differs from destination home; producer record kind disagrees with BIR instruction; ABI source differs from terminal home; or attached event differs from relation start |
| `Unsupported` | non-scalar result type; unsupported home; unsupported move op/authority/destination; unsupported scalar producer kind (including non-binary); unsupported ABI destination bank/placement; or unsupported freshness proof kind |
| `NonAdjacent` | the chain bundle is not at the next instruction boundary or the scalar producer is not exactly the next instruction |
| `WrongChainOperand` | neither binary operand names the prior value, both operands name it, or the resolved operand identity conflicts with the move source |
| `MissingFirstOperandHome` | the first non-chain operand is named but has no unique complete supported prepared home; non-named operands do not trigger this outcome |
| `CycleOrDepthExceeded` | a value/instruction repeats, progress is not strictly forward, or links exceed the block instruction count |
| `StructurallyIncomplete` | an otherwise available record lacks a required pointer/id/name, link list, chain role, selected freshness authority, terminal home, or terminal ABI binding payload |

All non-`Available` outcomes are non-consumable. Status classification remains
common and semantic; target ABI register conversion and instruction policy
begin only after a complete attached relation is returned.

## Suggested Next

- Execute Plan Step 2 by adding the status/query/link/relation types, composing
  them from existing prepared lookup producers, and attaching the result to
  starting instruction traversal events.

## Watchouts

- Keep traversal construction as the composition/attachment owner; do not move
  the AArch64 loop unchanged into a helper or make target code complete facts.
- The ABI terminal fact is the prepared before-return move, destination bank,
  and placement. Common code must not choose or spell an AArch64 register.
- Preserve a distinct valid state for a non-named first other operand versus a
  named operand whose home is missing.
- Authenticate freshness per chain link and fail closed on snapshot/proof
  mismatch; positional adjacency alone is insufficient.

## Proof

- Green exact delegated proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_object_consumer_contract$'`.
  Build was current and the focused test passed 1/1. Combined output is
  preserved in `test_after.log`; this proof is sufficient for the Step 1
  contract-only packet.
