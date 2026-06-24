# RV64 Vector Register Inline Asm Constraints Stage 2

## Goal

Implement the second child of
`ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`: RV64
vector register classes and inline asm constraints for `VR`, `VRM2`, and
`VRM4`.

This child builds on the completed Stage 1 scalar `.insn` route by making
vector operands first-class enough for later EV `.insn.d` encoding work.

## Why This Exists

The EV custom vector instruction route needs compiler-owned register
allocation and substitution for vector operands. Treating `VR`, `VRM2`, and
`VRM4` as opaque strings would let syntax pass while leaving the backend unable
to enforce alignment, occupancy, overlap, or stable base-register encoding.

Stage 2 establishes the vector register-class contract before introducing the
EV 64-bit `.insn.d` template.

## In Scope

- Recognize inline asm vector constraints:
  - `VR`
  - `=VR`
  - `+VR`
  - `VRM2`
  - `=VRM2`
  - `+VRM2`
  - `VRM4`
  - `=VRM4`
  - `+VRM4`
- Model vector register groups as physical-register classes:
  - `VR`: width 1, alignment 1, legal bases `v0` through `v31`
  - `VRM2`: width 2, alignment 2, legal bases `v0,v2,...,v30`
  - `VRM4`: width 4, alignment 4, legal bases `v0,v4,...,v28`
- Ensure distinct untied vector operands do not overlap occupied registers.
- Allow tied operands and read-write `+VR*` operands to reuse the intended
  register group according to inline asm rules.
- Substitute the selected base vector register for positional placeholders such
  as `%0`, not every occupied member of the group.
- Add focused tests for allocation, substitution, overlap rejection, impossible
  allocation, and unsupported or malformed vector constraints.

## Out Of Scope

- EV-specific `.insn.d` syntax or 64-bit custom instruction encoding.
- Named operands such as `[name] "VRM2"(x)` or `%c[name]` modifiers.
- Vector mask-specific constraints or policy bits.
- Broad RVV semantic lowering outside inline asm operand binding.
- Teaching c4c semantic meanings for EV custom operations.
- Runtime-generated asm template strings.
- Reworking the completed scalar `.insn` object-route proof except where the
  vector constraint path must share existing inline asm plumbing.

## Dependency Notes

- Parent umbrella:
  `ideas/open/339_rv64_inline_asm_custom_vector_encoding_umbrella.md`.
- Previous child:
  `ideas/open/340_rv64_standard_insn_inline_asm_stage1.md`.
- Stage 1 is parked as implementation-complete but not archived because the
  strict regression close gate is blocked by an unchanged unrelated backend
  baseline failure. This child should treat the scalar inline asm route as
  available foundation, not reopen Stage 1 lifecycle state.
- If vector operands need a source-language value representation, choose the
  narrowest viable proof fixture: opaque vector handles, existing vector types,
  or a backend-level fixture. Do not turn this child into broad vector
  language semantics work.

## Acceptance Criteria

- `VR`, `VRM2`, and `VRM4` constraints classify as vector register-class
  operands, not generic unsupported strings and not scalar GPR constraints.
- `VR` allocation can choose any legal vector register from `v0` through `v31`.
- `VRM2` allocation only chooses aligned bases and reserves both occupied
  registers.
- `VRM4` allocation only chooses aligned bases and reserves all four occupied
  registers.
- Untied vector groups cannot overlap, while tied or read-write operands reuse
  registers only when the inline asm constraint contract permits it.
- Placeholder substitution prints the base vector register for grouped
  operands.
- Impossible allocation and malformed or unsupported vector constraints fail
  explicitly.
- Existing scalar `.insn` behavior and tests are not weakened while adding
  vector constraints.

## Suggested Proof Ladder

- Build:

  ```bash
  cmake --build --preset default
  ```

- Narrow backend proof for vector constraint allocation, substitution, and
  diagnostics:

  ```bash
  ctest --test-dir build -j --output-on-failure -R '^backend_'
  ```

- Add frontend/HIR proof if the packet changes parser, HIR, or LIR inline-asm
  constraint carrier behavior:

  ```bash
  ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$|^frontend_cxx_'
  ```

## Reviewer Reject Signals

- The route only passes by marking vector inline asm tests unsupported or
  weakening existing scalar `.insn` expectations.
- `VR`, `VRM2`, or `VRM4` are accepted as raw strings but never become real
  vector register-class operands for allocation and substitution.
- `VRM2` or `VRM4` can allocate misaligned bases such as `v1` or reserve only
  the base register while leaving occupied neighbor registers free.
- Untied vector operands can overlap, or tied/read-write operands are forced
  apart in violation of inline asm semantics.
- Grouped operands substitute every member or a non-base register instead of
  the selected base vector register.
- The implementation adds EV `.insn.d`, named operands, mask policy semantics,
  or consteval-template string behavior before vector constraints are proven.
- The change claims progress through helper renames, expectation rewrites, or
  classification-only changes while the same vector allocation failure mode
  remains.
- Broad RVV lowering or unrelated backend rewrites dominate the slice instead
  of the focused inline asm vector constraint contract.

## Closure Note

Closed on 2026-06-24 after Stage 2 completed `VR`, `VRM2`, and `VRM4`
classification, allocation, overlap/tied-operand handling, base-register
substitution, and focused malformed/impossible-case diagnostics without
weakening the existing scalar `.insn` route.

Close proof used the canonical backend before/after logs:

```bash
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed
```

Result: PASS, with 315 passed, 1 known unrelated backend failure, 316 total in
both logs, and no new failures. The remaining EV `.insn.d`, named operand,
mask/policy, and broader RVV semantics work remains out of scope for this
closed child.
