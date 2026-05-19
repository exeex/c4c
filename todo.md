# AArch64 Fused Compare-Branch Operand Forms Todo

Status: Active
Source Idea Path: ideas/open/296_aarch64_fused_compare_branch_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Locate Operand Authority

# Current Packet

## Just Finished

Step 2 completed for focused idea 296. I traced the fused compare-branch
authority through AST-backed symbol queries and targeted reads of
`comparison.cpp`, `instruction.cpp/.hpp`, `machine_printer.cpp`, and the
branch-control / compare-record tests.

Operand authority is split across three layers:

- Semantic compare facts are owned by `prepare::PreparedBranchCondition`, then
  transcribed by `make_prepared_conditional_branch_record` into
  `BranchConditionRecord::{predicate, compare_operands,
  compare_branch_candidate}`. `BranchCompareCandidateRecord` mirrors those
  facts and fusion eligibility, but the printer does not consume it directly.
- Generic machine-node construction in `make_branch_instruction` calls
  `branch_instruction_operands`, which seeds a fused compare branch as five
  operands: true target, false target, condition value, lhs compare value, rhs
  compare value. At this point named compare values are still
  `PreparedValue` operands; immediates are immediate operands. This is enough
  for `branch_selection_status` to select `MachineOpcode::CompareBranch`, but
  it is not necessarily printable.
- Printable operand publication is owned by
  `install_fused_compare_print_operands` and
  `make_fused_compare_print_operand` in `comparison.cpp`. The installer is
  called from `lower_prepared_conditional_branch_terminator` after
  `make_branch_instruction`; it rewrites `instruction.target.operands[3]` and
  `[4]` from `condition.compare_operands` using `resolve_value_operand`.
  `print_fused_compare_branch` then consumes only those two operand slots for
  the `cmp` operands, while using `condition.predicate` and
  `condition.compare_operands->compare_type` only for condition code and
  register width.

The current c-testsuite path rejects printable operands at that boundary, not
because branch targets or candidate facts are missing. A fresh narrow run of
`c_testsuite_aarch64_backend_src_00030_c` still fails at function 0 block 4
instruction 3 with `opcode=compare_branch: fused compare branch operands are
not printable`. `--dump-prepared-bir` for `00030` shows that block 4 is the
semantic compare `slt i32 1000, %t8`, and prepared value locations give `%t8`
register authority (`x13`). That means the installer can publish the operands
literally as lhs immediate `1000` and rhs register `%t8`, but
`compare_operand_spelling` only accepts a register on the lhs and allows an
immediate only on the rhs. The record path preserves valid semantic compare
facts; the printable path lacks normalization for immediate-left compare
forms such as `imm < reg`, `imm <= reg`, `imm == reg`, and `imm != reg`.

Smallest viable Step 3 repair site: keep the fix at the fused compare
publication/printing boundary, centered on `install_fused_compare_print_operands`
and `print_fused_compare_branch`. The implementation packet should make
immediate-left forms publish or print a legal AArch64 `cmp` order by swapping
the printable operands and using the swapped predicate where required, while
leaving `BranchConditionRecord` / `compare_branch_candidate` as the durable
semantic facts. If the repair mutates stored branch condition facts, the
bounded site is `lower_prepared_conditional_branch_terminator` immediately
around the existing installer call, because that is where the fresh
`InstructionRecord` and its payload-owned `BranchConditionRecord` are both
available before machine printing. Avoid changing generic
`branch_instruction_operands`; its existing `PreparedValue` slots are useful
semantic/use records and the tests intentionally distinguish them from the
prepared-terminator route's printable register/immediate slots.

## Suggested Next

Delegate Step 3 to implement immediate-left fused compare-branch normalization
at the existing publication/printing boundary. A bounded implementation should
cover at least register-lhs/immediate-rhs, register-lhs/register-rhs, and
immediate-lhs/register-rhs forms without changing expectations or selecting
per-test behavior.

## Watchouts

- Do not treat this as a target-label or candidate-record problem: the
  representative failure has prepared branch-condition facts and register
  authority for the named operand.
- The current unit coverage proves the happy path for register-lhs plus
  immediate-rhs printing (`cmp w13, #0`) and the generic record path that still
  carries `PreparedValue` compare operands. It does not cover the `00030`
  immediate-left shape (`cmp #1000, w13` would be illegal/unprintable as
  currently modeled).
- Predicate swapping matters for ordered comparisons: `imm < reg` should print
  as `cmp reg, #imm` with the inverse relational condition, while equality and
  inequality can swap operands without changing the condition code.
- Do not match c-testsuite filenames, test numbers, or exact emitted
  instruction strings.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, or runner behavior.
- Keep this owner limited to fused compare-branch operand publication/printing;
  do not absorb the remaining scalar machine-printer, `lir_to_bir` admission,
  runtime, or timeout buckets from umbrella idea 295.
- Proving only one known failing case is not enough; the full 22-case focused
  family must be checked before claiming capability progress.

## Proof

Read-only proof only; `test_after.log` and `test_before.log` were not modified.
Commands/evidence used:

- AST-backed queries with `c4c-clang-tool-ccdb` for
  `lower_prepared_conditional_branch_terminator`,
  `install_fused_compare_print_operands`, `make_branch_instruction`,
  `print_fused_compare_branch`, and
  `lower_fused_compare_branch_from_emitted_cast`.
- Targeted source/test inspection of `comparison.cpp`, `instruction.cpp/.hpp`,
  `machine_printer.cpp`, `backend_aarch64_branch_control_lowering_test.cpp`,
  and `backend_aarch64_branch_compare_records_test.cpp`.
- Narrow local unit subset:
  `ctest --test-dir build -R 'backend_aarch64_(branch_control_lowering|branch_compare_records|compare_branch_candidate_records|prepared_branch_records|machine_printer)$' --output-on-failure`
  passed 5/5.
- Narrow representative c-testsuite check:
  `ctest --test-dir build -R '^c_testsuite_aarch64_backend_src_00030_c$' --output-on-failure`
  failed at the known fused compare-branch printer point.
- `./build/c4cll --dump-prepared-bir --target aarch64-linux-gnu --mir-focus-function main tests/c/external/c-testsuite/src/00030.c`
  showed block 4 as `slt i32 1000, %t8` with `%t8` register authority.
