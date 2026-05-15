Status: Active
Source Idea Path: ideas/open/243_inline_asm_tied_home_allocation_policy.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Harden Tests And Regression Scope

# Current Packet

## Just Finished

Step 5 is complete for backend inline-asm tied-home regression hardening:

- Added AArch64 dispatch fail-closed coverage for complete-looking tied homes
  that lack coallocation authority and for authority whose tied output index
  disagrees with the operand tie.
- Added selected inline-asm printer coverage for an incomplete selected tied
  output record, proving the tied input cannot print from a selected record
  that lacks structured selected-output authority.
- Existing supported alias-aware tied-home, operand/name/immediate/modifier,
  side-effect/output, memory/address, numeric-tie, and nearby fail-closed
  backend coverage remains green under the supervisor-selected backend subset.

## Suggested Next

No executor closure blocker remains in this packet. Supervisor should decide
whether Step 5 completion is enough for lifecycle review or whether an
independent reviewer pass is wanted before closure.

## Watchouts

- The dispatch coverage now distinguishes missing coallocation authority,
  authority output-index mismatch, and authority home mismatch as separate
  unsupported paths.
- The selected printer still validates copied prepared homes plus selected
  register identities; it does not carry `PreparedInlineAsmTiedHomeAuthority`
  directly and does not allocate locally.
- No expectation downgrades, skips, or named-case shortcuts were used.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof status: passed; build completed with no work to do and `ctest` reported
139/139 `backend_` tests passed. Proof log: `test_after.log`.
