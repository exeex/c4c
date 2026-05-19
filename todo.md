# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Failure Sources

# Current Packet

## Just Finished

Step 2 classified all 61 failures from accepted baseline `test_before.log`.
All failures are AArch64 c-testsuite backend tests; no local backend/unit/CLI
tests fail in the captured summary.

Classified inventory:

- Machine-printer frontend/prepared-module handoff failure: 22 tests.
  Common evidence is `[FRONTEND_FAIL]` followed by `AArch64 backend assembly
  route reached the machine-node printer, but printing failed: target
  instruction spelling failed`. Exact tests:
  - `1485 - c_testsuite_aarch64_backend_src_00024_c`: function 0 block 0,
    `scalar opcode=sub` cannot be printed.
  - `1497 - c_testsuite_aarch64_backend_src_00031_c`: function 0 block 4,
    `scalar opcode=add`; immediate outside plain `#imm` range `0..4095`.
  - `1503 - c_testsuite_aarch64_backend_src_00035_c`: function 0 block 0,
    `scalar opcode=zero_extend`; cast node requires supported integer width.
  - `1559 - c_testsuite_aarch64_backend_src_00064_c`: function 0 block 0,
    `scalar opcode=div` cannot be printed.
  - `1631 - c_testsuite_aarch64_backend_src_00104_c`: function 0 block 0,
    `scalar opcode=xor`; immediate outside plain `#imm` range `0..4095`.
  - `1633 - c_testsuite_aarch64_backend_src_00105_c`: function 0 block 3,
    `scalar opcode=zero_extend`; cast node requires supported integer width.
  - `1675 - c_testsuite_aarch64_backend_src_00126_c`: function 0 block 0,
    `scalar opcode=zero_extend`; cast node requires supported integer width.
  - `1691 - c_testsuite_aarch64_backend_src_00134_c`: function 0 block 0,
    `scalar opcode=sign_extend`; cast node requires a structured register
    source.
  - `1693 - c_testsuite_aarch64_backend_src_00135_c`: function 0 block 0,
    `scalar opcode=sign_extend`; cast node requires a structured register
    source.
  - `1701 - c_testsuite_aarch64_backend_src_00139_c`: function 0 block 0,
    `scalar opcode=mul` cannot be printed.
  - `1703 - c_testsuite_aarch64_backend_src_00140_c`: function 0 block 0,
    `deferred_unsupported: call-boundary move node is outside the selected`
    route; preserve as idea-297 printer residual.
  - `1725 - c_testsuite_aarch64_backend_src_00151_c`: function 0 block 0,
    `scalar opcode=zero_extend`; cast node requires supported integer width.
  - `1709 - c_testsuite_aarch64_backend_src_00143_c`: function 0 block 19,
    `scalar opcode=add`; immediate outside plain `#imm` range `0..4095`.
  - `1769 - c_testsuite_aarch64_backend_src_00173_c`: function 0 block 0,
    `memory opcode=store: stack-slot store source scratch is not printable`.
  - `1797 - c_testsuite_aarch64_backend_src_00187_c`: function 0 block 4,
    `memory opcode=store: stack-slot store source scratch is not printable`.
  - `1811 - c_testsuite_aarch64_backend_src_00194_c`: function 0 block 5,
    `memory opcode=store: stack-slot store source scratch is not printable`.
  - `1835 - c_testsuite_aarch64_backend_src_00207_c`: function 0 block 3,
    `scalar opcode=add`; immediate outside plain `#imm` range `0..4095`.
  - `1837 - c_testsuite_aarch64_backend_src_00208_c`: function 0 block 6,
    `scalar opcode=zero_extend`; cast node requires supported integer width.
  - `1849 - c_testsuite_aarch64_backend_src_00214_c`: function 0 block 4,
    `scalar opcode=xor`; immediate outside plain `#imm` range `0..4095`.
  - `1851 - c_testsuite_aarch64_backend_src_00215_c`: function 0 block 7,
    `scalar opcode=add`; immediate outside plain `#imm` range `0..4095`.
  - `1857 - c_testsuite_aarch64_backend_src_00218_c`: function 0 block 0,
    `scalar opcode=and`; immediate outside plain `#imm` range `0..4095`;
    preserve as idea-297 printer residual.
  - `1847 - c_testsuite_aarch64_backend_src_00213_c`: function 0 block 1,
    `scalar opcode=add`; immediate outside plain `#imm` range `0..4095`.

- Semantic `lir_to_bir` admission before prepared-module handoff: 8 tests.
  Common evidence is `[FRONTEND_FAIL]` plus `requires semantic lir_to_bir
  lowering before the prepared-module handoff`.
  - GEP/local-memory semantic family: `1775 -
    c_testsuite_aarch64_backend_src_00176_c` (`swap`), `1785 -
    c_testsuite_aarch64_backend_src_00181_c` (`PrintAll`), `1787 -
    c_testsuite_aarch64_backend_src_00182_c` (`print_led`), `1813 -
    c_testsuite_aarch64_backend_src_00195_c` (`main`), `1839 -
    c_testsuite_aarch64_backend_src_00209_c` (`f4`), `1831 -
    c_testsuite_aarch64_backend_src_00205_c` (`main`), and `1853 -
    c_testsuite_aarch64_backend_src_00216_c` (`foo`). Preserve `00216` as the
    known pointer-parameter/flexible-array aggregate projection residual even
    though the current coarse diagnostic says GEP local-memory.
  - Bootstrap/global admission: `1863 -
    c_testsuite_aarch64_backend_src_00204_c`; diagnostic says bootstrap
    `lir_to_bir` only supports scalar integer/pointer globals, linear
    integer-array globals, and aggregate-backed globals with honest
    byte-address semantics.

- Runtime nonzero exit/signaled process: 18 tests. Evidence is
  `[RUNTIME_NONZERO]` from the AArch64 backend runner, not frontend admission.
  Most have empty `stdout+stderr`, so the observed source is generated-program
  runtime failure rather than a printer or `lir_to_bir` block.
  - Silent nonzero exit: `1563 -
    c_testsuite_aarch64_backend_src_00066_c` (`exit=1`), `1645 -
    c_testsuite_aarch64_backend_src_00111_c` (`exit=1`), `1531 -
    c_testsuite_aarch64_backend_src_00050_c` (`exit=3`), `1661 -
    c_testsuite_aarch64_backend_src_00119_c` (`exit=1`), `1647 -
    c_testsuite_aarch64_backend_src_00112_c` (`exit=1`), `1595 -
    c_testsuite_aarch64_backend_src_00086_c` (`exit=1`), `1627 -
    c_testsuite_aarch64_backend_src_00102_c` (`exit=1`), `1649 -
    c_testsuite_aarch64_backend_src_00113_c` (`exit=1`), `1665 -
    c_testsuite_aarch64_backend_src_00121_c` (`exit=87`), `1669 -
    c_testsuite_aarch64_backend_src_00123_c` (`exit=1`), `1697 -
    c_testsuite_aarch64_backend_src_00137_c` (`exit=1`), `1699 -
    c_testsuite_aarch64_backend_src_00138_c` (`exit=1`), and `1711 -
    c_testsuite_aarch64_backend_src_00144_c` (`exit=1`).
  - Runtime signal/crash: `1601 -
    c_testsuite_aarch64_backend_src_00089_c` (`exit=Bus error`), `1763 -
    c_testsuite_aarch64_backend_src_00170_c` (`exit=Segmentation fault`),
    `1781 - c_testsuite_aarch64_backend_src_00179_c` (`exit=Segmentation
    fault`), and `1801 - c_testsuite_aarch64_backend_src_00189_c`
    (`exit=Segmentation fault`).
  - Nonzero with corrupted output evidence: `1823 -
    c_testsuite_aarch64_backend_src_00200_c` (`exit=27`), with repeated
    `((short)((1))) -1301144306 -1301144306` in `stdout+stderr`.

- Runtime output mismatch: 12 tests. Evidence is `[RUNTIME_MISMATCH]` from the
  backend runner, with expected/actual output recorded in the log.
  - `1737 - c_testsuite_aarch64_backend_src_00157_c`: expected squares
    `1..100`; actual contains large negative stack-looking values and zeros.
  - `1741 - c_testsuite_aarch64_backend_src_00159_c`: expected `9`, `16`,
    `a=1234`, `qfunc()`; actual has large negative numbers before `qfunc()`.
  - `1759 - c_testsuite_aarch64_backend_src_00168_c`: expected factorial
    sequence through `3628800`; actual is linear `1..10`.
  - `1761 - c_testsuite_aarch64_backend_src_00169_c`: expected nested triples;
    actual repeats a large negative middle field.
  - `1751 - c_testsuite_aarch64_backend_src_00164_c`: output matches early,
    then expected `1916` becomes a large negative value and `1915`.
  - `1767 - c_testsuite_aarch64_backend_src_00172_c`: expected `12`, `34`,
    `0`, `1`, `1`, `0`; actual includes large negative values.
  - `1795 - c_testsuite_aarch64_backend_src_00186_c`: expected `->01<-`
    through `->20<-`; actual only `->01<-`.
  - `1793 - c_testsuite_aarch64_backend_src_00185_c`: expected indexed values
    `0: 12` through `9: 9753` twice; actual zeros then repeated large negative
    indices.
  - `1773 - c_testsuite_aarch64_backend_src_00175_c`: expected char/int/float
    promotion outputs for `a`, `b`, `c`; actual corrupted chars, negative ints,
    and zero floats.
  - `1771 - c_testsuite_aarch64_backend_src_00174_c`: expected float/math and
    boolean rows; actual mostly zero floats and corrupted integer rows.
  - `1809 - c_testsuite_aarch64_backend_src_00193_c`: expected `1`, `2`,
    `out`, `3`; actual `out`, `out`, `out`.
  - `1815 - c_testsuite_aarch64_backend_src_00196_c`: expected `fred/joe`
    rows with `0/1`; actual includes large negative values and flipped
    booleans.

- Timeout/hang: 1 test. `1861 -
  c_testsuite_aarch64_backend_src_00220_c` timed out after `5.01 sec`; no
  failure diagnostic body was emitted before CTest reported `***Timeout`.

Known boundary notes:
- `1523 - c_testsuite_aarch64_backend_src_00046_c` is passing in
  `test_before.log`; do not reopen its closed owner from this failure set.
- `00140` and `00218` remain printer residuals in this capture.
- `00216` remains the pointer-parameter/flexible-array aggregate projection
  residual despite the current coarse GEP local-memory `lir_to_bir`
  diagnostic.

## Suggested Next

Proceed to Step 3: compare these classified buckets against closed owner
boundaries for ideas 285 through 297 before any split or reopen decision.

## Watchouts

- This is an umbrella inventory. Do not implement fixes before a focused owner
  is split and activated.
- Preserve closed-owner boundaries for ideas 285 through 297 unless current
  generated-code, proof, or diagnostic evidence contradicts them.
- The fresh failing list is entirely `aarch64_backend c_testsuite`; no local
  backend/unit/CLI failures appear in the CTest failure summary.
- `c_testsuite_aarch64_backend_src_00220_c` timed out in the fresh run and
  should be treated as a timeout/hang classification input, not folded into a
  generic failure bucket without evidence.
- Treat residual global/pointer/aggregate GEP work, `00216`
  pointer-parameter/flexible-array aggregate projection, and printer residuals
  as classification inputs, not automatic new owners.
- The runtime nonzero and runtime mismatch groups are observed runner outcomes
  only. This packet did not inspect generated assembly deeply enough to claim a
  semantic repair owner for those failures.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, implementation files, or
  canonical logs during lifecycle activation.
- `test_before.log` is the accepted focused baseline. `test_after.log` is
  absent at activation, and `test_baseline.log` is a pre-existing ignored root
  log.

## Proof

Inspection-only packet. Source of truth was accepted repo-root
`test_before.log`; no tests were run and no canonical logs were modified.
Classification accounts for all 61 failures in the CTest summary:
22 machine-printer, 8 `lir_to_bir` admission, 18 runtime nonzero, 12 runtime
mismatch, and 1 timeout.
