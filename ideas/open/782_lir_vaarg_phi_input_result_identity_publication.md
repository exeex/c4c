# LIR Vaarg PHI Input Result Identity Publication

Status: Open
Type: bounded LIR vaarg producer prerequisite
Blocked Consumer: `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
Context: closed `ideas/closed/775_lir_phi_producer_helper_result_identity.md`

## Goal

Publish native, current-function typed result/value identity for every
intermediate helper value that enters one of the three existing vaarg PHI
constructors.

## Why This Exists

Closed 775 established `LirVaArgOp.result` for the later vaarg operation. That
fact does not identify the raw intermediate values used as PHI inputs. The
existing AArch64 GP and FP helpers and AMD64 join pass display spellings such
as `reg_addr`, `stack_ptr`, `aligned_stack_ptr`, `reg_value`, and `stack_value`
to their PHI constructors without native IDs. 751 therefore cannot publish its
typed incoming carrier for the vaarg family without this producer-only handoff.

## In Scope

- Establish native current-function typed result fields for the raw values
  supplied to the PHI constructors in `emit_aarch64_vaarg_gp_src_ptr`,
  `emit_aarch64_vaarg_fp_src_ptr`, and the AMD64 vaarg join.
- Carry those fields alongside compatibility spelling through only the helper
  operations needed to reach each of those existing PHI inputs.
- Add focused positive and malformed-authority coverage proving every named
  vaarg PHI input has valid native result/value identity and that missing,
  invalid, duplicate, or foreign authority fails closed.
- Publish a precise handoff naming the typed input fields and the three covered
  constructors so 751 can resume its one PHI carrier/verifier slice.

## Out Of Scope

- `LirPhiOp`, PHI verification, predecessor or edge authority, and any typed
  PHI incoming carrier work.
- Raw-BIR/importer, backend, target lowering, MIR, emission, or unrelated
  vaarg result/value paths.
- Using names, labels, rendered text, instruction order, or testcase text to
  recover identity; side tables and result-name maps are also excluded.
- Claiming that closed 775's later `LirVaArgOp.result` identifies these helper
  sources or completes the PHI consumer work.

## Acceptance Criteria

- Every raw value entering the existing AArch64 GP, AArch64 FP, and AMD64
  vaarg PHI constructors has a native typed, current-function-owned helper
  input result field before compatibility spelling is used.
- Focused positive coverage reaches each covered PHI input structurally, and
  malformed missing, invalid, duplicate, and foreign fields reject closed.
- The accepted handoff identifies every covered constructor and typed input
  field, with no `LirPhiOp` or downstream authority claim.
- 751 can resume exactly at Step 1 with the vaarg input facts available beside
  its already accepted ternary and logical producer facts.

## Reviewer Reject Signals

- Reject parsing `%` names, labels, rendered LLVM/printer output, instruction
  order, or testcase text to manufacture a helper-input ID.
- Reject using `LirVaArgOp.result` as a substitute for the intermediate values
  that enter the vaarg PHIs.
- Reject side tables, result-name maps, synthetic values, named-test branches,
  expectation downgrades, or weaker malformed-authority contracts.
- Reject changes to `LirPhiOp`, PHI verifier, predecessor/edge authority,
  Raw-BIR/importer, backend, target lowering, MIR, or emission as progress.
- Reject a GP-only, FP-only, or AMD64-only change claimed to cover all existing
  vaarg PHI constructors.
