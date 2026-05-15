Status: Active
Source Idea Path: ideas/open/250_i128_deferred_helper_family_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Source-Operation And Helper Mapping

# Current Packet

## Just Finished

Executed Step 2 by adding prepared/shared source-operation mapping records for
supported i128 float/integer conversion helpers.

The prepared helper model now records cast-origin helper facts beside existing
div/rem helper facts: optional source cast opcode, source/result byte widths,
source/result signedness, and a single conversion operand value identity. The
producer now maps supported F32/F64 <-> I128 `FPToSI`, `FPToUI`, `SIToFP`, and
`UIToFP` casts into `PreparedI128RuntimeHelper` records with
`FloatIntegerConversion` family, helper kind, callee identity, function/block/
instruction source identity, operand/result value identities, and source/result
type facts. F128/binary128 or otherwise unsupported conversion shapes remain
fail-closed through explicit deferred diagnostics.

The prepared printer now renders conversion helper mappings structurally,
including cast opcode, operand identity, widths, and signedness. Existing
direct-result div/rem helper-call printing and AArch64 selected-record/printer
paths were not changed.

## Suggested Next

Execute Step 3 as the prepared/shared ABI and result-ownership packet for the
new conversion helper family. Define which supported conversion helpers have
direct low/high i128 result lanes versus scalar FP result ownership, which
source operands require i128 carrier lanes versus FPR scalar authority, and
which memory-return-required or F128/binary128 cases remain explicitly
deferred. Keep AArch64 selected nodes and terminal printer output out of that
packet unless the supervisor delegates them.

Suggested focused proof for Step 3:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- Do not reopen direct-result div/rem helper-call printing unless a concrete
  regression is found.
- Conversion helpers now have source-operation/helper mapping records, but
  boundary policy, ABI register-bank transitions, lane/scalar result ownership,
  marshaling, clobber/resource policy, live-preservation, and selected-call
  ownership remain incomplete by design.
- Do not treat the existing memory-return vocabulary as ownership. I128 helper
  memory-return support still lacks destination/slot/offset producer authority
  and helper ABI policy tying an sret/memory destination to the helper record.
- Do not hard-code helper ABI registers, fixed low/high lane pairs, register
  adjacency, rendered names, or helper callee strings in AArch64 target
  lowering.
- Do not lower helper-required operations as scalar i64 shortcuts.
- Preserve existing supported div/rem helper-call behavior while adding
  deferred helper authority.

## Proof

Focused proof passed:

```bash
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: 3/3 tests passed. `git diff --check` also passed.

Supervisor full-suite acceptance also passed: supervisor ran
`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1`
and the regression guard against `test_before.log` copied from
`test_baseline.log` passed with before 3167/3167 and after 3167/3167.
