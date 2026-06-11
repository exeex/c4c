# Return-Chain Owner/Schema Decision Facts

Status: Step 2 owner/schema decision complete.

Source idea: `ideas/open/176_return_chain_owner_schema_decision.md`

## Existing Data Shape

`PreparedReturnChainLookups` currently exposes two maps:

- `terminal_return_values_by_chain_value`
- `next_operand_values_by_chain_value`

Both maps use `prepared_return_chain_value_key(block_index, instruction_index,
value_name)`, so the stable lookup identity is a BIR block position, BIR
instruction position, and the named result value at that position. The map
payloads are `ValueNameId` answers only; no register, home, ABI, scratch, or
emission-order data is stored in the prepared return-chain lookup.

The terminal map answers: for this chain value, which named value is the return
terminator's terminal return value?

The next-operand map answers: for this chain value, which named value is the
other operand of the immediate next binary in the same chain, when that operand
has a prepared name?

## Producer Accepted Shapes

`make_prepared_return_chain_lookups(...)` accepts only same-block scalar binary
chains that end at a named return terminator value:

- The prepared BIR function and block must resolve.
- The BIR block terminator must be `Return`.
- The return terminator must have a named value with an existing prepared name.
- The candidate instruction must be a `bir::BinaryInst`.
- The candidate opcode must be a scalar publication opcode in the prepared
  whitelist: `Add`, `Sub`, `And`, `Or`, `Xor`, `Shl`, `LShr`, `AShr`, `Mul`,
  `SDiv`, `SRem`, `UDiv`, or `URem`.
- The candidate binary result must be named and must have an existing prepared
  name.
- Chain traversal starts from the candidate result name and walks only later
  instructions in the same block.
- Each next instruction in the walk must also be a whitelisted scalar binary,
  must consume the current chain value as either lhs or rhs, and must have a
  named result.
- The first successful next binary contributes `next_operand_value_name` from
  the other operand if that other operand has an existing prepared name.
- Publication happens only if the final walked result name matches the return
  terminator's named value.

## Fail-Closed And Ambiguous Cases

The producer emits no answer, or invalidates a conflicting answer, for these
cases:

- Missing prepared BIR function or block.
- Non-return terminator, return without a value, or return of an unnamed value.
- Return value, candidate result, or first next operand without an existing
  prepared name.
- Candidate instruction that is not a binary instruction.
- Candidate or next binary opcode outside the scalar publication whitelist.
- Candidate binary result or next binary result that is not named.
- Same-block chain walk broken by a non-binary instruction, unsupported opcode,
  next binary that does not consume the current chain value, or unnamed next
  result.
- Walked chain value that does not match the return terminator value.
- Duplicate publication for the same `(block_index, instruction_index,
  value_name)` key with conflicting terminal or next-operand answers; the
  conflicting map entry becomes `kInvalidValueName`.

Consumer-side failures are also fail-closed: return-chain answers are ignored
when prepared lookups, current BIR block, value locations, terminal home,
return ABI move/register conversion, next operand home, next operand AArch64
register parsing, scalar register view, or scratch allocation are unavailable.

## AArch64 Consumer Expectations

`find_return_chain_register(...)` uses the terminal answer only after ordinary
return ABI register lookup for the current binary result has failed. It expects:

- `context.function.prepared_lookups`, `context.bir_block`, and
  `context.function.value_locations` to be present.
- A terminal answer for `(context.block_index, instruction_index,
  result_home.value_name)`.
- A prepared value home for the terminal value.
- A before-return ABI move for the terminal value and destination bank implied
  by the BIR return value type.
- Successful AArch64 register conversion for that ABI move.

When those conditions hold, AArch64 reuses the terminal return ABI register and
retargets the `RegisterOperand` metadata to the intermediate binary result
value id, name, and type. The semantic lookup chooses the terminal value
identity; the target code chooses the ABI register and retargeting behavior.

`lower_scalar_instruction(...)` uses the next-operand answer only in the
fallback scalar binary path after a result register has been found. It expects:

- The current result home to be a `RematerializableImmediate`.
- Authoritative immediate storage for the current binary result.
- Prepared return-chain lookup and value locations to be present.
- A next-operand answer for the current result.
- A prepared value home for that next operand.
- A parseable AArch64 register name for the next operand home.

If the next operand's AArch64 register aliases the selected result register,
the consumer chooses a reserved GP scratch register that does not alias the
result register and retargets that scratch to the current result value id/name.
If no alias is detected, the original result register remains selected.

## Target-Policy Boundaries

The prepared return-chain lookup currently owns only target-neutral identity:

- chain key: block index, instruction index, chain value name
- terminal answer: return terminator value name
- next-operand answer: first next binary's other operand value name, when named

AArch64 owns the target policy around those answers:

- value-home lookup and missing-home handling
- return ABI destination bank choice
- before-return ABI move lookup
- prepared-register to AArch64-register conversion
- parsing AArch64 register names from homes
- alias detection between next operand and result registers
- scalar register view choice
- reserved scratch register selection
- final scalar ALU record construction and emission order

Any BIR-owned schema candidate should preserve only the identity relationship
above and must not import AArch64 homes, registers, ABI return placement,
scratch choice, final instruction records, or final instruction order.

## Step 2 Decision

Owner choice: BIR-owned return-chain identity.

The terminal-value and next-operand answers are target-neutral enough to become
a BIR route because the stable payload is only BIR identity:

- the current function and block
- the instruction position of a scalar binary result inside that block
- the named chain value produced at that instruction
- the named return terminator value reached by the chain
- the named other operand of the immediate next binary in the chain, when such
  an operand exists and has a stable prepared/BIR name

The current AArch64 uses of those answers are target-specific, but the answers
themselves do not encode a register, home, ABI destination, scratch, instruction
record, or emission order. This makes the relationship comparable to the
existing BIR route pattern: BIR owns semantic identity and cheap lookup keys;
target lowering owns policy and materialization.

### Proposed BIR-Owned Records

A follow-up implementation should model this as a distinct return-chain route,
not as Route 1 producer identity or Route 7 comparison provenance.

The durable record should be attached to the binary instruction result or to a
function-local index that references that instruction result. The minimum
semantic payload is:

- `block` identity for the owning BIR block
- `instruction_index` or an equivalent stable instruction reference
- `chain_value` as the named result value produced by that instruction
- `terminal_return_value` as the named value consumed by the return terminator
- optional `next_operand_value` as the named other operand of the immediate next
  binary in the same chain
- status for unavailable/conflicting answers, represented as absent or invalid
  rather than target-specific fallback data

The lookup key should preserve the current cheap-read shape:

```text
function, block, instruction_index, chain_value
```

If a future BIR schema has stable instruction ids, the route may use that id
instead of a raw index, but the record must still validate that the referenced
instruction belongs to the same BIR block and produces `chain_value`.

### Accepted Cases

The BIR route should accept only the semantic subset already proven by Step 1:

- same-function, same-block chains
- block terminator is `Return`
- return terminator consumes a named value
- candidate instruction is a scalar `BinaryInst`
- candidate result is named
- candidate opcode is one of the scalar publication opcodes currently accepted
  by the prepared producer: `Add`, `Sub`, `And`, `Or`, `Xor`, `Shl`, `LShr`,
  `AShr`, `Mul`, `SDiv`, `SRem`, `UDiv`, or `URem`
- traversal starts from the candidate result and walks later instructions in
  the same block
- every walked next instruction is a named-result scalar binary from the same
  opcode set and consumes the current chain value
- the first next-binary other operand is recorded only when it has a stable
  value name
- publication succeeds only when the final walked result is the value consumed
  by the return terminator

### Rejected And Fail-Closed Cases

The BIR route must reject or produce no answer for:

- non-return terminators
- returns without a value or returns of unnamed values
- candidate instructions that are not scalar binary instructions
- unsupported opcodes
- unnamed candidate results or unnamed walked results
- broken same-block walks, including intervening non-binary instructions,
  unsupported next opcodes, next binaries that do not consume the current chain
  value, or unnamed next results
- walked chains whose final value is not the return terminator value
- next-operand publication when the other operand has no stable name
- duplicate records for the same key with conflicting terminal or next-operand
  answers
- any case that would require inspecting target homes, registers, ABI return
  placement, scratch availability, or final emitted instruction order to decide
  the semantic answer

Conflicts should fail closed. The route should not pick one conflicting answer
based on traversal accident, target lowering state, or consumer preference.

### Stability And Boundary Rules

- The route is function-local and block-local; it must not describe chains that
  cross block boundaries.
- The route identity is over BIR values and instruction positions/references,
  not over prepared homes or target operands.
- Instruction-index keys are valid only against the exact BIR block ordering
  used to build the route. Any transformation that reorders, inserts, deletes,
  or renames relevant BIR instructions must rebuild or invalidate the index.
- Value names in the record must refer to BIR values in the same function and
  must be checked against the owning block/instruction/terminator.
- Missing homes, missing before-return ABI moves, missing target register
  conversions, register aliasing, scratch allocation failure, or scalar ALU
  record construction failure are target-lowering failures, not BIR-route
  failures.
- The route may expose a function-local index for cheap lookup, but that index
  must not become a generic lowering-plan aggregate or duplicate target policy.
- Existing prepared helpers should remain public as migration oracles until
  BIR/prepared equivalence is proven for positive, negative, ambiguous, and
  conflict cases.

### Rejected Owner Choices

Target-local AArch64-owned identity is rejected as the durable owner. AArch64
should continue to own the register/home/scratch/emission decisions that use
the relationship, but future x86 or RISC-V lowering should not have to
rediscover the same same-block return-chain value relation from target code.
Duplicating the relation per target would make the semantic accept/reject
matrix drift while still producing target-neutral value-name answers.

Explicitly public prepared-owned identity is rejected as the end-state owner.
The prepared API can remain public during migration and oracle proof, but the
relationship is not inherently a prepared allocation product: its durable
meaning is a BIR value chain ending at a BIR return terminator. Keeping it as
the permanent public owner would preserve the current aggregate-coupling
pressure without a target-policy reason.

Route 1 and Route 7 ownership are also rejected. Return-chain identity uses
same-block producer and scalar binary facts, but its semantic answer is a chain
relationship to a return terminator plus an immediate next-operand relation.
It is not merely a producer-source lookup and not comparison or branch
condition provenance.

## File Anchors

- `src/backend/prealloc/prepared_lookups.hpp`: `PreparedReturnChainLookups`
  maps and public find helpers.
- `src/backend/prealloc/prepared_lookups.cpp`: key construction at
  `prepared_return_chain_value_key(...)`, producer whitelist and traversal at
  `make_prepared_return_chain_lookups(...)`, and conflict handling at
  `publish_prepared_return_chain(...)`.
- `src/backend/mir/aarch64/codegen/alu.cpp`: terminal consumer
  `find_return_chain_register(...)`; next-operand consumer in
  `lower_scalar_instruction(...)`.
