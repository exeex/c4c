Status: Active
Source Idea Path: ideas/open/127_lir_structured_function_signature_metadata_boundary.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Add Drift-Resistance Tests

# Current Packet

## Just Finished

Completed `plan.md` Step 5, `Add Drift-Resistance Tests`.

Converted the aarch64 direct-LIR fast-path signature gates away from
`function.signature_text` parsing:

- Return-type checks now use `signature_return_type_ref` before the legacy
  `return_type` fallback.
- Fixed-parameter and variadic checks now use `signature_params`,
  `signature_param_type_refs`, `signature_is_variadic`, and
  `signature_has_void_param_list` before the legacy `params` fallback.
- Signature nonminimal-type gating now checks structured signature return and
  parameter type refs instead of parsing the rendered function header.
- Added `backend_aarch64_signature_metadata` as a backend drift guard covering
  return type, fixed-parameter count, variadic status, and nonminimal
  signature gating.

## Suggested Next

Supervisor should review whether the active runbook is exhausted and whether
remaining `signature_text` consumers are expected legacy compatibility paths or
need a follow-up idea.

## Watchouts

The aarch64 source is not currently wired into `c4c_backend`, so the new
backend test is a static drift guard over `emit.cpp` rather than a runtime
aarch64 emission test. BIR still has a text parser for legacy hand-built LIR
fixtures with missing structured signature fields; generated LIR should not
take that path once Step 2 metadata is present.

## Proof

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`
passed. Proof log: `test_after.log`.
