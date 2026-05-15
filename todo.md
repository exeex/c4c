Status: Active
Source Idea Path: ideas/open/243_inline_asm_tied_home_allocation_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Accept Proven Alias-Aware Tied Homes In AArch64 Selection

# Current Packet

## Just Finished

Step 4 is complete without adding redundant selected-record fields:

- The current selected `PreparedValueHome::target_register_identity` bridge
  already carries proven alias-aware tied-home authority through AArch64
  dispatch into `InlineAsmMachineOperandRecord::home`.
- AArch64 dispatch accepts tied inputs only after
  `PreparedInlineAsmTiedHomeAuthority` agrees with the tied output index and
  the target-normalized shared register identity, then selects the tied register
  from the prepared home rather than local allocation policy.
- The selected inline-asm printer prints the accepted tied operand from the
  selected structured operand while revalidating the selected input/output
  register identities against the prepared homes, so selected records without
  proven shared-home authority remain fail-closed.

## Suggested Next

Proceed to Step 5 by hardening the regression scope around the existing
alias-aware accepted representative and nearby fail-closed cases.

## Watchouts

- `InlineAsmMachineOperandRecord` still intentionally does not carry
  `PreparedInlineAsmTiedHomeAuthority` directly; dispatch consumes that authority
  before selection, and the printer validates the selected record through copied
  prepared homes plus selected register identities.
- Keep allocator-dependent, missing, target-invalid, class-invalid, mismatched,
  and authority-mismatched tied homes on the explicit unsupported path.
- Do not add printer-local allocation or rendered-text matching in Step 5.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof status: passed; build completed with no work to do and `ctest` reported
139/139 `backend_` tests passed. Proof log: `test_after.log`.
