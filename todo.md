Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.41
Current Step Title: Receive The One 829-Authorized Body-Parameter Argument-1 Row

# Current Packet

## Just Finished

Completed Step 7.41. Raw BIR now receives closed 829's
`FixedDirectCallArgument1` DirectScalar body-parameter authority row, preserving
the selected source value, owner, parameter index, argument index, scalar type,
and direct-call argument/signature coherence through the call node. The
receiver keeps the accepted argument-0 row and rejects malformed argument-1
authority transactionally.

## Suggested Next

Return to the 734 source completion gate. Do not infer source completion from
this bounded receiver row alone; reassess the no-omission matrix and identify
the next first-owner blocker or receiver-ready row.

## Watchouts

- Do not edit LIR producer/schema/verifier publication for this packet.
- Do not recover identity from text, names, rendered operands, diagnostics,
  signature strings, or compatibility mirrors.
- Do not receive any other body-parameter form, call argument index, generic
  call, variadic/indirect/unspecified call, ABI conversion, memory/VA,
  aggregate/vector, module/type/global/metadata, residual
  instruction/terminator, or inline-assembly family.
- Closed 829 makes no Raw-BIR receipt claim; this packet owns that receipt.

## Proof

Passed matching focused regression proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_before.log 2>&1`

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_to_bir_interface$' ) > test_after.log 2>&1 && git diff --check`

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: before 1/1, after 1/1, no new failures; `git diff --check` passed.
