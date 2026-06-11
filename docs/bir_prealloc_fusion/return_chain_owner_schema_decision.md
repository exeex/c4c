# Return-Chain Owner/Schema Decision Facts

Status: Step 1 inventory complete.

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
