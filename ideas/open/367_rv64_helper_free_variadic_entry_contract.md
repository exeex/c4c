# RV64 Helper-Free Variadic Entry Contract

Status: Open
Type: Target ABI follow-up
Parent: `ideas/closed/363_rv64_object_parameter_home_coverage.md`

## Goal

Define the RV64 object-route contract for helper-free variadic function entries
so representative cases such as `src/20030914-2.c` can advance past prepared
function admission without bypassing the explicit variadic runtime contract.

## Why This Exists

Idea 363 proved the focused byval aggregate parameter-home diagnostic, but its
representative rerun stopped before parameter-home admission at:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```

That boundary is earlier than the parameter-home class and belongs to RV64
variadic entry admission/runtime-contract work, not to parameter-home coverage.

## In Scope

- Audit the prepared facts available for helper-free RV64 variadic entries.
- Define when a helper-free variadic entry is admissible under the current
  object route, or keep it rejected with a precise structured diagnostic.
- Add focused backend coverage for the selected admission contract.
- Rerun `src/20030914-2.c` as a representative signal after the focused work.

## Out of Scope

- Implementing scalar or aggregate `va_arg`, `va_copy`, or `va_start` helper
  lowering beyond facts already supplied by prior variadic ideas.
- Implementing byval aggregate parameter-home lowering in prepared stack slots;
  that boundary is documented by idea 363.
- Rewriting RV64 object admission around a specific torture testcase shape.
- Improving scan results through expectation downgrades, skip broadening, or
  allowlist filtering.

## Acceptance Criteria

- Focused backend tests prove the helper-free variadic entry admission contract
  or the precise unsupported diagnostic.
- Existing RV64 variadic helper and parameter-home coverage remains green.
- `src/20030914-2.c` is rerun and its next boundary is documented honestly.
- Progress is based on prepared variadic entry/runtime facts, not testcase
  names or source-shape matching.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c` or matching its exact
  source shape instead of defining a real helper-free variadic entry contract.
- Reject weakening or removing the explicit variadic runtime-contract gate
  without focused tests that prove the replacement contract.
- Reject expectation downgrades, skip broadening, or allowlist filtering
  claimed as variadic entry progress.
- Reject moving `va_arg`, `va_copy`, or parameter-home work into this idea
  unless the audited helper-free entry contract directly requires it.
- Reject diagnostic-only renames that leave the same helper-free variadic entry
  admission boundary unexplained behind new wording.

## Starting Signal

- Representative case: `src/20030914-2.c`
- Current boundary:

```text
unsupported_function_admission: RV64 helper-free variadic entry lowering remains unsupported without an explicit supported variadic entry runtime contract
```
