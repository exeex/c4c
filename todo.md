Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct post-298 residual inventory

# Current Packet

## Just Finished

Step 1: Reconstruct post-298 residual inventory. Fresh `ctest -R backend`
inventory was captured in `test_after.log` after a current default-preset build.

Residual inventory from `test_after.log`:

- Local backend/unit failures: none observed. The selected local/internal
  backend tests in this run passed; all 60 residual failures are under
  `c_testsuite_aarch64_backend_*`.
- c_testsuite machine-printer/frontend residuals: 28 total.
  - Scalar add/sub/bitwise immediate outside plain `#imm` range:
    `00031`, `00104`, `00143`, `00207`, `00213`, `00214`, `00215`,
    `00218`.
  - Scalar cast printable operand/width gaps: `00035`, `00105`, `00126`,
    `00134`, `00135`, `00151`, `00208`.
  - Scalar mul/div/rem printable operand gaps: `00064`, `00139`.
  - Immediate-lhs scalar subtract printable gap: `00024`.
  - Call-boundary move outside selected register subset: `00140`.
  - Memory store source/symbol printable gaps: `00173`, `00176`, `00181`,
    `00182`, `00187`, `00194`.
  - Semantic `lir_to_bir` admission failures: `00204`, `00216`.
  - Scalar unsigned reduction printable gap: `00205`.
- c_testsuite runtime nonzero residuals: 18 total.
  - `exit=1`: `00066`, `00086`, `00102`, `00111`, `00112`, `00113`,
    `00119`, `00123`, `00137`, `00138`, `00144`.
  - Segmentation fault: `00089`, `00170`, `00179`, `00189`.
  - Other nonzero exits: `00050` (`exit=3`), `00121` (`exit=231`),
    `00200` (`exit=27`).
- c_testsuite runtime mismatch residuals: 13 total: `00157`, `00159`,
  `00164`, `00168`, `00169`, `00172`, `00174`, `00175`, `00185`,
  `00186`, `00193`, `00195`, `00196`.
- Timeout residuals: `00220` timed out at the per-test timeout.

## Suggested Next

Best next focused-owner candidate: scalar add/sub/bitwise immediate materialize
or encoding fallback for AArch64 machine printing. It has the largest crisp
machine-printer diagnostic bucket (`00031`, `00104`, `00143`, `00207`,
`00213`, `00214`, `00215`, `00218`) and a clear shared semantic signal:
selected scalar add/xor/and nodes reach the printer with immediates outside the
plain `#imm` encoding range instead of being materialized or lowered through a
printable fallback.

## Watchouts

- Do not implement fixes under this umbrella plan.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, or CTest registration.
- Do not reopen closed owners 285 through 298 from failing counts alone.
- Runtime scans need timeout handling and stale-process cleanup.
- Runtime nonzero/mismatch buckets are not ready as focused owners from this
  inventory alone; they need generated assembly or narrower probes before a
  semantic owner can be declared.
- Treat timeout `00220` as separate hang/quarantine evidence unless later proof
  ties it to another focused semantic bucket.

## Proof

Ran:

- `cmake --build --preset default`
- `timeout 900s ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1`

Result: build was current; CTest exited 8 with 60 failures out of 352 selected
tests (`83% tests passed`). `test_after.log` is the fresh inventory proof.
Stale-process hygiene: checked before and after the broad runtime command; no
stale qemu/c4c runtime process was found or terminated.
