Status: Active
Source Idea Path: ideas/open/586_uniform_target_register_identity_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extend Shared Identity Publication

# Current Packet

## Just Finished

Step 2 from `plan.md` extended shared ABI target register identity publication
in `src/backend/prealloc/target_register_profile.cpp`.

Implementation notes:
- Preserved the existing RV64 behavior in
  `target_register_identity_for_abi_register_placement(...)`: `CallArgument`
  and `CallResult` `Gpr`/`Fpr` placements with slots `0..7` still map to
  physical indexes `10..17`.
- Added AArch64 scalar ABI identity publication for `CallArgument` and
  `CallResult` placements with width `1` and slots `0..7`: `Gpr` and
  `AggregateAddress` publish concrete GPR identity `x0..x7`, while `Fpr` and
  `Vreg` publish FP/SIMD identity `0..7`.
- Added x86-64 scalar ABI identity publication for stable SysV placements:
  argument `Gpr`/`AggregateAddress` slots `0..5` map to physical indexes
  `{7, 6, 2, 1, 8, 9}` for `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`; argument
  `Fpr` slots `0..7` map to `xmm0..xmm7`; result `Gpr`/`AggregateAddress`
  slot `0` maps to `rax`; result `Fpr` slot `0` maps to `xmm0`.
- New AArch64/x86-64 paths fail closed for `I686`, non-ABI pools, `None`
  banks, zero-width placements, out-of-range slots, vector x86 ABI placements,
  and multi-register/contiguous shapes. AArch64 `x8` sret remains
  identity-less because there is still no shared placement policy for it.

## Suggested Next

Execute Step 3 from `plan.md`: adapt the narrow prepared/prealloc or backend
consumers that need ABI physical identity to use the shared publication surface
where appropriate, without broad call-lowering rewrites.

## Watchouts

- Do not change semantic ABI classification, value freshness authority,
  preservation fallback, move-bundle authority, broad backend lowering, test
  expectations, unsupported markers, allowlists, or runtime-comparison
  behavior under this idea.
- AArch64 `x8` sret pointer still has no placement and no identity; keep it
  fail-closed unless a later lifecycle step explicitly adds placement policy.
- x86-64 physical-index numbering is now an explicit shared policy contract for
  the supported SysV ABI argument/result registers; consumers should not
  reinterpret ABI slot order as physical register numbering.
- `AggregateAddress` stable ABI placements publish concrete GPR identity, not
  a distinct physical aggregate-address register class.

## Proof

Command run:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. Proof log: `test_after.log`.
