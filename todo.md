Status: Active
Source Idea Path: ideas/open/243_inline_asm_tied_home_allocation_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Harden Fail-Closed Tied-Home Boundaries

# Current Packet

## Just Finished

Step 3 hardened the fail-closed tied-home boundaries:

- AArch64 dispatch now validates target-normalized tied input/output identities,
  register class compatibility, home agreement, authority output index, and
  authority shared-register payload before it selects a tied inline-asm operand.
- The selected inline-asm printer now rejects tied records whose selected input
  or output register payload disagrees with the structured prepared home instead
  of printing from the selected operand alone.
- Coverage now includes dispatch fail-closed cases for target-invalid tied homes,
  incompatible register classes, mismatched homes, mismatched authority payloads,
  allocator-dependent homes, missing homes, and missing tied authority, while
  existing concrete and alias-aware supported ties still pass.

## Suggested Next

Proceed to Step 4 by carrying proven alias-aware tied-home authority through
AArch64 selection and printing as the accepted path, while keeping the Step 3
fail-closed cases intact.

## Watchouts

- `InlineAsmMachineOperandRecord` still does not carry
  `PreparedInlineAsmTiedHomeAuthority` directly, so the printer boundary validates
  selected operands against the structured prepared homes available on the
  selected record.
- Preparation remains allocator-policy-free: allocator-dependent tied homes still
  fail closed instead of inserting moves or repairing allocation.
- Keep dispatch/printer checks based on `target_register_identity` and prepared
  authority payloads; do not fall back to rendered register text.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof status: passed; build completed and `ctest` reported 139/139 `backend_`
tests passed. Proof log: `test_after.log`.
