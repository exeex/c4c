Status: Active
Source Idea Path: ideas/open/243_inline_asm_tied_home_allocation_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Tied-Home Coallocation Authority

# Current Packet

## Just Finished

Step 2 added structured tied-home coallocation authority for prepared
inline-asm homes:

- `PreparedValueHome` now carries an optional target-normalized register
  identity, and prepared inline-asm tied operands record explicit shared-home
  authority only after both sides normalize to the same target-valid GPR home.
- AArch64 inline-asm preparation normalizes `wN`/`xN` aliases to the same
  physical GPR identity, rejects target-invalid/class-incompatible/mismatched
  tied homes with structured facts, and leaves allocator-dependent homes
  fail-closed.
- AArch64 dispatch now requires the prepared tied-home authority instead of
  comparing rendered register strings, and the printer validates selected tied
  operands through structured target-home identity rather than raw text.
- Coverage now includes alias-aware `w3`/`x3` tied homes, preserved same-register
  concrete ties, selected-printer fail-closed checks, and a prepared-carrier
  mismatch case that records `tied_input_output_home_mismatch` instead of being
  treated as complete.

## Suggested Next

Proceed to Step 3 by hardening the remaining fail-closed boundaries around
missing authority, target-invalid tied register names, incompatible register
classes, and allocator-dependent tied homes before selection or printing can
infer a home.

## Watchouts

- Current preparation proves coallocation only when both prepared homes already
  carry concrete target-normalized identities; it does not introduce allocator
  repair or move insertion for mismatched homes.
- A semantic prepared-carrier example whose allocator places the tied input in
  `x0` and the output in `x21` now correctly remains incomplete with
  `tied_input_output_home_mismatch`.
- Keep using structured `PreparedValueHome::target_register_identity` and
  `PreparedInlineAsmTiedHomeAuthority`; do not reintroduce raw
  `register_name` equality in dispatch or printer checks.

## Proof

Supervisor-selected proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Proof status: passed; build completed and `ctest` reported 139/139 `backend_`
tests passed. Proof log: `test_after.log`.
