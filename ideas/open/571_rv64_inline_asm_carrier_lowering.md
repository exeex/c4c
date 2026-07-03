# RV64 Inline Asm Carrier Lowering

Status: Open
Type: Capability repair
Parent: `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
Owning Layer: RV64 object lowering for inline asm carrier calls

## Goal

Implement or explicitly model the RV64 object-route handling needed for BIR
`CallInst` nodes whose callee is `llvm.inline_asm`, so inline asm carrier calls
stop falling through to the generic unsupported instruction fallback.

## Why This Exists

The Step 3 diagnostics from the 570 runbook split four retained
`unsupported_instruction_fragment` representatives into the same first-owner
family:

- `src/20071211-1.c`
- `src/pr51933.c`
- `src/pr56982.c`
- `src/pr78438.c`

Each case reaches BIR, prepared BIR, and MIR, then fails in RV64 object
lowering at a `CallInst` with `owner=none` and an `llvm.inline_asm` callee.
`src/pr78438.c` is especially important because its source shape can look like
an arithmetic or shift issue, but the first object-route blocker is the inline
asm carrier call.

Evidence:

- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/classification.tsv`
- `build/agent_state/570_unsupported_instruction_fragment_diagnostics/summary.md`
- Per-case logs under
  `build/agent_state/570_unsupported_instruction_fragment_diagnostics/`

## In Scope

- Inspect the BIR and prepared-BIR representation of `llvm.inline_asm`
  carriers on RV64.
- Define the RV64 object-emission contract for side-effecting inline asm calls
  with no value owner, including memory clobber handling where represented.
- Handle symbol-address operands and ordinary GPR operands only to the extent
  required by the inline asm carrier contract.
- Add focused object-route tests that prove the carrier path no longer reaches
  generic `unsupported_instruction_fragment` for this family.
- Preserve a precise diagnostic for inline asm forms that remain unsupported
  after the carrier work.

## Out Of Scope

- General RV64 same-module or external function call ABI lowering.
- Pointer arithmetic, select, floating-point binary, or branch-published phi
  lowering.
- Runtime comparison, expected-output rewrites, unsupported marker edits, or
  allowlist changes.
- Testcase-specific matching for the four representative filenames.
- Claiming `src/pr78438.c` is fixed through arithmetic or shift work before
  the inline asm carrier blocker is addressed.

## Acceptance Criteria

- The inline asm carrier representatives either lower through the RV64 object
  route or fail with a narrower inline-asm-specific diagnostic that identifies
  the unsupported constraint/form.
- Focused backend tests cover at least a no-result memory-clobber carrier and
  the symbol-address operand shape seen in `src/pr51933.c`.
- Existing diagnostic category stability is preserved where unsupported cases
  remain unsupported.
- No unrelated RV64 lowering families are modified as part of the proof.

## Reviewer Reject Signals

- Reject a slice that matches `20071211-1.c`, `pr51933.c`, `pr56982.c`, or
  `pr78438.c` by filename or exact source shape.
- Reject treating inline asm carrier progress as same-module call ABI progress,
  pointer arithmetic progress, or shift/arithmetic progress.
- Reject expectation downgrades, unsupported-marker additions, allowlist edits,
  or runtime-output changes used as the main proof.
- Reject helper renames or diagnostic-only wording changes claimed as carrier
  lowering progress.
- Reject a broad call-lowering rewrite that leaves `llvm.inline_asm` carriers
  falling through to the old generic `unsupported_instruction_fragment` path.
