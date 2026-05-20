Status: Active
Source Idea Path: ideas/open/338_aarch64_scalar_cast_register_source_operand_facts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Scalar Cast Operand Fact Loss

# Current Packet

## Just Finished

Step 1 localized the scalar cast operand-fact loss to the selected-node source
operand publication/normalization boundary in `lower_scalar_cast_instruction`.

Representative evidence:

- `00143` fails with `opcode=sign_extend` at function 0 block 3 instruction 2.
  The prepared BIR for `main` at that boundary is `%t8 = bir.sext i32 %t7 to
  i64`; prepared storage has source `%t7` as `frame_slot` and result `%t8` as
  register `x13`. Prepared value locations also contain a before-instruction
  consumer move from `%t7` to `%t8` with destination storage `register`, but
  the selected scalar cast source remains the frame-slot/memory operand and the
  printer receives no structured register source.
- `00216` fails with `opcode=zero_extend` at function `print_`, block 3
  instruction 4. The prepared BIR is `%t9 = bir.zext i8 %t8 to i32`; prepared
  storage has source `%t8` as `frame_slot` and result `%t9` as register `x13`.
  Prepared value locations contain the matching before-instruction
  `consumer_stack_to_register` move from `%t8` to `%t9`, but selected cast
  lowering still hands the printer a non-register source.
- AST/text evidence: `make_prepared_scalar_operand` converts prepared
  frame-slot operands into `MemoryOperand` records; `make_prepared_scalar_cast_record`
  stores that operand in `ScalarCastRecord::source`; `lower_scalar_cast_instruction`
  only rewrites the cast source when `find_emitted_scalar_register` already has
  a register for the named operand. It does not consume the prepared
  before-instruction stack-to-register move, so the selected record's
  `scalar_cast.source` and `inputs[0]` are stale before `make_scalar_instruction`.

Decision: the missing source belongs to selected scalar cast operand
normalization/fact publication, not printer consumption. The printer can print
register-sourced scalar casts and is correctly rejecting the selected cast
record it receives because the selected-node handoff never publishes the
prepared consumer register as the structured source operand.

## Suggested Next

Delegate Step 2 to repair `lower_scalar_cast_instruction` so register-sourced
`sign_extend` and `zero_extend` selected nodes publish the prepared
consumer/register source operand into both `scalar_cast.source` and
`inputs[0]` before `make_scalar_instruction`.

## Watchouts

- Do not reopen closed owners 334 through 337 from counts alone.
- Keep the scope on scalar cast register-source operand facts, not generic
  scalar arithmetic operand printing.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, or CTest registration.
- If the old printer diagnostic disappears but representatives still fail,
  classify the new first bad fact before claiming closure.
- Avoid a printer-only workaround. The evidence points before printing: the
  selected scalar cast handoff ignores an already-planned consumer register move
  for frame-slot sources.

## Proof

Representative localization commands:

- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00143.c -o /tmp/c4c_00143.s`
- `./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00216.c -o /tmp/c4c_00216.s`
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00143.c`
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function print_ tests/c/external/c-testsuite/src/00216.c`

Delegated proof: `git diff --check` passed; output preserved in
`test_after.log`.
