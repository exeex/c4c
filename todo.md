Status: Active
Source Idea Path: ideas/open/248_prepared_i128_runtime_helper_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Source Operation To Helper Mapping

# Current Packet

## Just Finished

Step 2 implemented prepared/shared source-operation-to-helper mapping authority
for i128 div/rem only.

What changed:

- Added `PreparedI128RuntimeHelpers` beside prepared call plans and i128
  carriers, with function-scoped `PreparedI128RuntimeHelper` records.
- Each div/rem helper record carries function, block index, instruction index,
  `bir::BinaryOpcode`, source/result type facts, result value id/name, lhs/rhs
  value id/name, helper family/kind, and explicit compiler-rt callee identity.
- Populated mappings during prepared regalloc publication for i128 `SDiv`,
  `UDiv`, `SRem`, and `URem`:
  `__divti3`, `__udivti3`, `__modti3`, and `__umodti3`.
- Added explicit deferred diagnostics for i128 float/integer conversion cast
  mapping without creating conversion helper records in this packet.
- Extended prepared printer output with a
  `--- prepared-i128-runtime-helpers ---` section.
- Added focused BIR prepared tests proving structural div/rem mapping,
  callee/opcode/type/value identity preservation, printer output, and deferred
  float/i128 conversion status.

No AArch64 target records, terminal printer output, lane bindings,
memory-return ownership, clobber/resource policy, fixed-register marshaling, or
scalar-i64 substitutes were added.

## Suggested Next

Execute Step 3 as a prepared/shared lane-authority packet: extend
`PreparedI128RuntimeHelper` records with low/high argument lane bindings and
direct low/high result lane bindings for the div/rem helper families, consuming
existing `PreparedI128Carrier` facts rather than inferring lanes from register
names or adjacency.

Suggested focused proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

## Watchouts

- The new helper records intentionally stop at source-operation mapping:
  low/high lanes, memory-return ownership, clobbers/resources, and ABI
  transition facts are still missing by design.
- `PreparedCallPlan` remains retained-call-only. Do not synthesize or reuse a
  fake call plan for these non-call `BinaryInst` helpers without producer-owned
  source-operation identity.
- Float/i128 conversion helpers are explicitly deferred via prepared missing
  facts; the next packet should not treat conversion mappings as complete.
- The population path is prepared-only and runs from regalloc-owned source
  operation/value authority; idea 236 AArch64 helper selection must continue to
  wait for later lane and boundary policy facts.

## Proof

Passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_liveness|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_prepare_liveness`,
`backend_prepare_frame_stack_call_contract`, and `backend_prepared_printer`
passed, 3/3 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 2 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.
