Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Validate And Summarize

# Current Packet

## Just Finished

Step 8 validated the accepted AArch64 i128 pair-lowering coverage and recorded
the remaining helper-boundary limits.

Accepted coverage:

- Prepared/shared state exposes explicit i128 carrier authority for register
  pairs and memory-backed values, with value identity, low/high lane ordering,
  lane width, total size/alignment, placement provenance, and missing-fact
  diagnostics.
- AArch64 selection consumes complete `PreparedI128Carrier` facts into selected
  transport records without inferring lanes from register names, numeric
  adjacency, rendered names, or fixed `x0`/`x1` conventions.
- Supported i128 add/sub/and/or/xor, immediate-count shifts, and equality plus
  signed/unsigned comparisons select into structured pair records preserving
  result/source carrier identity, lane order, operation semantics, shift-count
  facts, and comparison signedness/high-word semantics.
- Prepared i128 div/rem helper authority exists for `SDiv`, `UDiv`, `SRem`,
  and `URem`: source-operation mapping, helper/callee identity, low/high
  argument lanes from canonical carriers, direct low/high result ownership,
  resource/clobber policy, and ABI/register-bank shape facts.
- AArch64 selection consumes those prepared div/rem helper facts into
  `I128RuntimeHelperBoundaryRecord` values and keeps missing helper authority,
  incomplete policy, unsupported carrier kinds, non-direct result ownership,
  conversion helpers, and memory-return helper families fail-closed.
- Terminal AArch64 printing is supported for structured i128 memory transports,
  add/sub/bitwise pair operations, immediate-count 0..63 shifts, and
  equality/signed/unsigned comparison records.
- Terminal div/rem helper-boundary call printing is intentionally not claimed:
  `I128RuntimeHelperBoundaryRecord` currently fails closed in the printer with
  an explicit missing structured helper marshaling/ABI register-binding
  diagnostic.

No operand recovery from rendered names, fixed `x0`/`x1` conventions,
source-opcode-only helper synthesis, runtime-library changes, helper-call
printing, float/i128 conversion support, or memory-return helper-family support
was added.

## Suggested Next

Hand idea 236 to plan-owner close review. If plan-owner requires executable
div/rem helper-call terminal output before closing the source idea, the next
implementation packet should first add structured helper marshaling and ABI
register-binding authority, then consume those facts in the AArch64 printer.

## Watchouts

- i128 shift printer support is the immediate-count 0..63 subset. Register
  counts and 64+ count expansion remain fail-closed and should not be papered
  over with name-shaped cases.
- Div/rem helper-boundary selected records still require structured marshaling
  and ABI register-binding facts before terminal `bl <callee>` output can be
  accepted. Lane registers and ABI shape/banks are not sufficient proof of
  executable helper-call printing.
- Float/i128 conversion helpers and memory-return helper families remain
  deferred until prepared/shared authority is specified for those shapes.
- `PreparedCallPlan` remains retained-call-only; i128 helper-required source
  operations are represented by prepared helper authority plus selected helper
  boundary records, not fake call plans.

## Proof

Focused Step 7 proof passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_target_instruction_records|backend_aarch64_target_record_core_contract|backend_aarch64_mir_carrier|backend_aarch64_instruction_dispatch|backend_aarch64_machine_printer)$') > test_after.log 2>&1
```

Result: build succeeded; `backend_aarch64_target_instruction_records`,
`backend_aarch64_target_record_core_contract`, `backend_aarch64_mir_carrier`,
`backend_aarch64_instruction_dispatch`, and `backend_aarch64_machine_printer`
passed, 5/5 tests. Proof log: `test_after.log`.

Additional hygiene: `git diff --check` passed.

Supervisor full-suite acceptance also passed for this Step 7 slice:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Regression guard used `test_before.log` copied from accepted
`test_baseline.log`; result was 3167/3167 before and 3167/3167 after.

Step 8 full-suite validation passed:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

Result: build succeeded; full suite passed, 3167/3167 tests. Proof log:
`test_after.log`.
