# LLVM Torture 20040709 Owner Discovery

## Scope

This package documents owner discovery for:

- `llvm_gcc_c_torture_src_20040709_2_c`
- `llvm_gcc_c_torture_src_20040709_3_c`

The research scope is documentation only. It does not change compiler code,
test expectations, unsupported markers, allowlists, runtime behavior, baseline
policy, or lifecycle history.

## Answers

- [Current failure boundary](01_current_failure_boundary.md): both rows
  currently reproduce as generated-program runtime failures. Clang-built
  binaries exit successfully, while the generated `c2ll` binaries abort after
  the compile pipe succeeds.
- [Owner mapping](02_owner_mapping.md): neither row maps to an existing
  generated implementation follow-up on current evidence. The shared runtime
  symptom is not enough to assign either row to a closed RV64, prepared,
  object-emission, AArch64, callee-saved, or packed-member owner.

## Result

The research package answers the active question: the two LLVM torture
`20040709` rows do not have a proven existing generated implementation owner.
A later implementation effort should begin with row-specific evidence gathering
for the generated-binary aborts, then name the first concrete owner before any
repair work starts.

Until that evidence exists, the accepted result is no owner assignment and no
implementation, expectation, allowlist, unsupported-marker, runtime, or
baseline-policy change under this research idea.
