# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.27
Current Step Title: Publish insn-r positional operand authority

## Just Finished

- Stopped Plan Step 7.26 without code changes and classified generic scalar
  read/write inline-asm publication as an exact separately owned blocker.
- Paired native `ReadWrite` roles and indices can encode distinct old/new IDs,
  but cannot distinguish generic `+r` from the excluded explicit-register
  `+{reg}` route. HIR/LIR retain that constraint/register-class distinction
  only in opaque text.
- Restricting verification by parsing that text is forbidden, while publishing
  the common shape would silently widen the packet. Truthful classification
  therefore requires a new native constraint/register-class carrier owned by a
  separate future inline-assembly schema initiative.
- Step 7 remains active: `.insn r` has native structured operand indices that
  can identify its positional ordinary inputs/result without consulting
  constraint text. That is a distinct carrier-ready production disposition.
- Parity, multi-output/input-only inline assembly, and producer-specific
  implicit coercion retain their exact separately owned blockers.

## Suggested Next

- Executor: complete Plan Step 7.27 for only one scalar i32
  `LirInlineAsmOp` carrying native `insn_r` metadata with positional rd/rs1/rs2
  operand indices. Preserve two already-authoritative current-function scalar
  inputs at the exact rs1/rs2 positions, allocate the distinct rd semantic
  result through `fresh_value`, and preserve that result ID into one later
  type-matched Store.

## Watchouts

- Own only ordinary value identity for the three positions named by native
  `insn_r.operand_indices`: one i32 result/definition at rd and two i32
  input/uses at rs1 and rs2. Require exact index, role, position/count, type,
  uniqueness, and current-function ownership agreement.
- The focused fixture may source rs1/rs2 from selected-global loads, but the
  semantic contract is any valid authoritative current-function i32 producers;
  do not invent selected-global provenance or reject another valid i32 source.
- The rd result must be fresh and distinct from both inputs, and the later
  Store must consume that exact result ID. Claim only the Store value-use edge,
  not general pointer/object or Store authority.
- Native `insn_r` opcode/function fields and positional indices remain exact.
  Original/rendered assembly, constraint text, register names/classes,
  clobbers, compatibility result, and other mirrors remain opaque or
  presentation-only and must not create, select, repair, or validate IDs.
- Reachable verification must reject missing/wrong-alternative inputs or
  result; invalid, duplicate, aliased, unknown, or cross-function IDs; malformed
  native R metadata or operand indices; binding role/type/index/count/position
  conflicts; and wrong final Store ID/type. Misleading displays and constraint
  spelling changes with unchanged native facts must not affect identity proof.
- Exclude generic non-`insn_r` inline assembly, constraint/register-class
  semantics, read/write old/new pairing, multi-output, memory/address/immediate/
  clobber bindings, floating/vector/aggregate operands, stack/local/object/body
  parameters, CFG, BIR receipt, parity, and idea-741 changes.
- This packet is carrier-ready only on native `insn_r` positional metadata,
  existing scalar binding facts, `fresh_value`, and generic ownership. If exact
  rd/rs1/rs2 publication still needs constraint-text interpretation or a new
  carrier, stop and return that blocker rather than widening Step 7.27.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with exact rs1/rs2 input IDs, a distinct rd result-to-Store edge,
  misleading-display/constraint positives, and malformed native R metadata,
  position/index/count, identity, role/type, and final-use cases.
- Preserve Steps 7.21/7.25 output-only behavior, existing `.insn r` metadata/
  diagnostics, selected-global load contracts, and accepted generic identity
  coverage unchanged; the supervisor owns matched regression and broader proof.
- Run `git diff --check` before handoff.
