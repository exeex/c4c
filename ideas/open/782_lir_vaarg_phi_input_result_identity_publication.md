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

## Resumption Record: native vaarg operand/result seam decomposition blocker

Last accepted progress: no 782 implementation slice, after-proof, or
implementation commit has been accepted. Completed runbook steps: none. The
interrupted step is Step 1, `Publish native vaarg PHI-helper input fields`;
Step 2 is unstarted.

The first bad verifier fact is `LirGepOp.ptr`. Making the AArch64 `reg_addr`
helper GEP authoritative requires an authoritative raw SSA base (`gr_top`),
which recursively reaches the generic vaarg expression/operand chain. This is
outside 782's producer-only helper-input scope. The prior 751 route likewise
showed that its PHI carrier cannot invent these IDs without forbidden text
recovery. Across those two PHI-facing routes, the first bad fact moved upstream
without reducing the vaarg failure family; the required route is therefore a
separate decomposition initiative, not a PHI-carrier or verifier repair.

Classification: `separate-blocker`. Open
`ideas/open/783_lir_native_vaarg_operand_result_seam_decomposition.md` must
first establish a focused frontend-LIR baseline and enumerate/bind the native
vaarg operand/result contracts for the AArch64 GP GEP-address, AArch64
FP/alignment-helper, and AMD64 reg/stack-helper chains. It must not modify the
PHI carrier or verification and must not absorb Raw-BIR/importer, backend,
target lowering, MIR, or emission work.

Exact return point: once 783 accepts the narrowest viable native vaarg
operand/result contract for all three chains, reactivate 782 at Step 1 and
publish its bounded helper-input fields. Only after 782 completes may 751
resume at its recorded Step 1. The remaining work in 782 is otherwise
unchanged.

Accepted proof: the restored baseline command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`
passed 1/1, with matching `test_before.log` and `test_after.log` guard pass
under `--allow-non-decreasing-passed`. Accepted implementation commit
references: none; no code changes were accepted.

## Resumption Update: 783 decomposition accepted

783 closed capability-complete after accepting the AArch64 GP `gr_top` pointer
load to indexed `reg_addr` GEP, AArch64 FP ptrmask result to aligned-stack GEP,
and AMD64 register-GEP/stack-load to immediate-`memcpy` native contracts. The
accepted implementation references are `e45a6b0ee` and `2d16c90dc`; the
focused frontend-LIR command passed 1/1 after a fresh build and the matching
`test_before.log`/`test_after.log` guard passed under
`--allow-non-decreasing-passed`.

Exact return point remains Step 1, `Publish native vaarg PHI-helper input
fields`. No 782 implementation slice is complete. Use the accepted three-chain
contracts as upstream authority only; publish 782's bounded helper-input fields
without extending into PHI carrier/verifier, predecessor/edge, CFG,
Raw-BIR/importer, backend, target lowering, MIR, emission, or generic
migration.

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
