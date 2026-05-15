Status: Active
Source Idea Path: ideas/open/239_aarch64_intrinsic_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Structured Intrinsic Carrier Facts

# Current Packet

## Just Finished

Step 2: `Define Structured Intrinsic Carrier Facts`.

Added explicit target-neutral intrinsic metadata for BIR `llvm.fabs` calls and
published prepared intrinsic carrier facts for scalar FP unary F32/F64 `fabs`
only. Complete carriers preserve family, operation, operand/result type,
operand/result identity, side-effect and feature facts, source call provenance,
and the presence of the authoritative `PreparedCallPlan`.

The prepared printer now exposes complete intrinsic carriers in a dedicated
debug section and emits missing-fact diagnostics for incomplete carriers. It
does not print incomplete or unsupported carriers as usable records.

Fail-closed coverage now proves that missing result facts, missing prepared
call-result facts, x86/F128 `fabs`, and ordinary call plans without structured
BIR intrinsic metadata do not become selected-intrinsic authority.

## Suggested Next

Execute Step 3 by consuming only complete prepared scalar FP unary F32/F64
`fabs` carriers into AArch64 selected intrinsic records. Keep ordinary
`PreparedCallPlan` facts insufficient for intrinsic selection and preserve
F128, x86-only, CRC/vector/barrier/cache/hint, and builtin-address families as
unsupported until they have explicit structured carrier authority.

## Watchouts

- Do not claim AArch64 intrinsic support through intrinsic-name string matching or archived scratch-register conventions.
- Keep binary128 helpers, atomics, and inline asm outside this plan.
- Unsupported x86-only intrinsics must remain rejected, trapped, or explicitly diagnosed rather than silently zero-filled.
- Ordinary `PreparedCallPlan` facts are still not sufficient intrinsic
  authority; selected intrinsic support must require a complete prepared
  intrinsic carrier.
- x86/F128 `fabs` currently carries a structured diagnostic but is
  intentionally not a complete intrinsic carrier because binary128 remains
  delegated to the closed binary128 route.
- CRC, vector, barrier/cache/hint, and builtin-address families still need
  structured semantic feature/operand facts before this route can accept them.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log`

Result: passed, `139/139` backend tests. Proof log: `test_after.log`.
