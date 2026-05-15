Status: Active
Source Idea Path: ideas/open/236_aarch64_i128_pair_lowering.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Print Supported I128 Pair Nodes

# Current Packet

## Just Finished

Step 7 added terminal AArch64 printer support for the supported structured i128
selected-record subset.

What changed:

- `machine_printer.cpp` now prints selected i128 transport records from
  structured low/high lane registers and structured memory operands. Supported
  memory transports emit `ldp`/`stp`; carrier snapshots print only a structured
  diagnostic comment.
- Selected i128 add/sub/and/or/xor pair records print from structured
  result/lhs/rhs low/high register fields. Add/sub use carry/borrow-aware
  `adds`/`adc` and `subs`/`sbc`; bitwise operations emit independent low/high
  lane instructions.
- Selected immediate-count i128 shifts in the 0..63 subset print from
  structured result/source pair registers and count facts.
- Selected equality and signed/unsigned relational i128 comparisons print from
  structured result register and low/high pair operands.
- Selected div/rem `I128RuntimeHelperBoundaryRecord` values now fail closed in
  the terminal printer with an explicit diagnostic that structured helper
  marshaling and ABI register-binding facts are missing. The record's lane and
  ABI-shape facts are preserved for future consumption, but the printer no
  longer emits `bl <callee>` from lane facts alone.
- Machine-printer tests cover structured i128 transport, pair add, shift,
  equality and unsigned compare output, helper-boundary fail-closed diagnostics
  for missing marshaling/register-binding authority, and incomplete pair lane
  facts.

No operand recovery from rendered names, fixed `x0`/`x1` conventions,
source-opcode-only helper synthesis, runtime-library changes, helper-call
printing, float/i128 conversion support, or memory-return helper-family support
was added.

## Suggested Next

Execute Step 8 validation for idea 236. The validation packet should run the
supervisor-selected full-suite proof, summarize semantic coverage for i128
carrier authority, selected transport/arithmetic/shift/compare/helper records,
terminal printer support, and remaining deferred helper families, then hand the
route back for plan-owner close review if coverage is accepted.

Suggested full-suite proof:

```sh
(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure) > test_after.log 2>&1
```

## Watchouts

- i128 shift printer support is the immediate-count 0..63 subset. Register
  counts and 64+ count expansion remain fail-closed.
- Div/rem helper-boundary selected records still require a future structured
  marshaling/register-binding packet before terminal call output can be
  claimed. Lane registers and ABI shape/banks are not sufficient proof of
  executable helper-call printing.

## Proof

Passed:

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
