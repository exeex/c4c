Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 2 from `plan.md` classified the fresh post-309
`c_testsuite_aarch64_backend_` residuals from supervisor-captured
`test_after.log`.

Current broad c-testsuite backend scope:

- Command captured by supervisor:
  `timeout 240s ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_' > test_after.log 2>&1`
- Result: 212 selected, 154 passed, 58 failed.
- `c_testsuite_aarch64_backend_src_00189_c` now passes in this fresh broad
  post-309 capture and remains excluded from the umbrella residual set.

Primary residual buckets by failure source:

- Runtime nonzero: 34 tests:
  `00023.c`, `00026.c`, `00035.c`, `00051.c`, `00062.c`, `00066.c`,
  `00067.c`, `00068.c`, `00069.c`, `00070.c`, `00086.c`, `00089.c`,
  `00095.c`, `00096.c`, `00102.c`, `00110.c`, `00111.c`, `00112.c`,
  `00113.c`, `00119.c`, `00123.c`, `00137.c`, `00138.c`, `00142.c`,
  `00144.c`, `00151.c`, `00170.c`, `00173.c`, `00179.c`, `00181.c`,
  `00200.c`, `00207.c`, `00208.c`, and `00215.c`.
- Runtime mismatch: 16 tests:
  `00157.c`, `00159.c`, `00168.c`, `00169.c`, `00172.c`, `00174.c`,
  `00175.c`, `00176.c`, `00182.c`, `00185.c`, `00186.c`, `00193.c`,
  `00195.c`, `00196.c`, `00205.c`, and `00218.c`.
- Timeout / no explicit runner tag because CTest killed the runner: 3 tests:
  `00143.c`, `00187.c`, and `00220.c`.
- Frontend / machine-printer / semantic handoff failures: 5 tests:
  `00140.c`, `00164.c`, `00204.c`, `00214.c`, and `00216.c`.

Frontend sub-buckets:

- Machine-printer prepared-node/call-boundary move: `00140.c`:
  `call-boundary move node is outside the selected register call-boundary move subset`.
- Machine-printer scalar mul/div/rem printable-rhs facts: `00164.c`:
  `scalar mul/div/rem node has incomplete printable rhs facts`.
- Machine-printer prepared frame-slot source for scalar add/sub/bitwise memory
  operands: `00214.c`:
  `scalar add/sub/bitwise memory operands require prepared frame-slot sources`.
- Semantic `lir_to_bir` local-memory handoff: `00204.c` fails in GEP
  local-memory lowering for `myprintf`; `00216.c` fails in load local-memory
  lowering for `foo`.

Output-storm note:

- `00181.c` is still primarily `RUNTIME_NONZERO exit=Segmentation fault`, but
  it emitted about 2,094,067 stdout/stderr lines before failing. Treat it as a
  runtime crash plus output-storm case for future bounded proof routing.
- `00200.c` is `RUNTIME_NONZERO exit=26` with a smaller repeated-output block.
  It is not in the same output-storm class as `00181.c` by current evidence.
- No non-timeout failed block lacked a runner tag; all 55 non-timeout failures
  carried either `RUNTIME_NONZERO`, `RUNTIME_MISMATCH`, or `FRONTEND_FAIL`.

Absent buckets in this fresh c-testsuite capture:

- Compile failures: 0.
- Assembler failures: 0.
- Linker failures: 0.
- Old closed-owner `00189.c` indirect-call residual: absent; `00189.c` passed.

## Suggested Next

Step 3 can consider a focused owner, but only one candidate is clean enough
from this classification alone: the singleton `00140.c` AArch64
call-boundary move prepared-node/machine-printer gap. It has a crisp frontend
diagnostic, narrow proof surface, and does not require reopening closed owners
285 through 309 from counts alone.

Keep the broader runtime nonzero/mismatch buckets parked until a narrower
generated-code probe identifies a shared semantic owner. Keep `00204.c` /
`00216.c` semantic `lir_to_bir` local-memory failures parked or probe them
before splitting, because prior closed local/global projection work means
counts alone are not enough to infer a reopened owner. Treat `00181.c` as a
bounded-proof/output-storm routing concern before any runtime-focused packet.

## Watchouts

- This umbrella packet is classification only; do not implement repairs here.
- Do not reopen closed owners 285 through 309 from counts alone.
- Do not change expectations, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or proof-log policy.
- The current 58-failure count is for `^c_testsuite_aarch64_backend_` only,
  not full `ctest -R backend`; do not compare it directly to historical
  352-test backend-regex totals.
- Runtime failures are heterogeneous by current evidence. Do not merge direct
  call shuffle, direct vararg, address-of-local, crash, mismatch, timeout, and
  output-storm cases without generated-code evidence.
- The machine-printer/frontend failures are not one owner by current evidence:
  `00140.c`, `00164.c`, and `00214.c` have different diagnostics and should
  stay distinct unless a later probe proves a shared selected-node preparation
  rule.
- If Step 3 chooses `00140.c`, keep the owner semantic: selected
  call-boundary move preparation/printing, not filename matching or an
  instruction-spelling workaround.

## Proof

No build or CTest was run by this executor, per the classification-only packet.
Read-only evidence used: supervisor-captured `test_after.log`, `plan.md`,
`ideas/open/295_backend_regex_failure_family_inventory.md`, and small source
samples for the likely candidate/frontend boundary cases. `test_after.log` was
not written or modified.
