Status: Active
Source Idea Path: ideas/open/614_rv64_pointer_local_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Split Or Close-Readiness Classification

# Current Packet

## Just Finished

Executed Step 4, `Residual Split Or Close-Readiness Classification`, for idea
614. No implementation, plan, idea, expectation, allowlist, unsupported-marker,
timeout, or accounting files were changed.

Residual evidence inspected:

- Reused the Step 3 residual probe artifacts under
  `build/agent_state/614_step3_residual_refresh/`.
- Candidate set: `35` prior `unsupported_local_memory_access` rows from
  `build/agent_state/612_step3_failure_reason_rows.tsv`.
- Probe command recorded by Step 3:
  `ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist STOP_ON_FAILURE=0 VERBOSE_FAILURES=0 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/614_step3_residual_refresh/probe.log 2>&1 || true`
- Result remained `0/35` passed; all `35` still stop at
  `unsupported_local_memory_access`.

Close-readiness classification:

- Close/retire idea 614 as an RV64 selected pointer/local-memory consumer
  route. The completed frame-slot consumer slice was bounded semantic progress,
  and the refreshed residuals do not expose another in-scope family with
  complete selected authority and meaningful breadth.
- Do not implement another 614 consumer packet from the remaining rows. The
  remaining failures are better described as policy or producer/publication
  gaps, unsupported width coverage, aggregate stack-home policy, or unusually
  large selected offsets, not as straightforward RV64 consumption of already
  complete selected local-memory authority.

Residual split recommendations:

- Create or reuse a durable string-constant local-memory policy route for rows
  with `string_constant` local-memory bases: `src/20000722-1.c`,
  `src/20010123-1.c`, `src/20011109-2.c`, `src/20021204-1.c`,
  `src/20030920-1.c`, `src/920429-1.c`, `src/930429-1.c`,
  `src/pr34415.c`, `src/pr35800.c`, and `src/ptr-arith-1.c`.
- Create or reuse a durable direct global-symbol local-memory policy route for
  rows with `global_symbol` local-memory bases. Existing idea 621 covers
  prepared global value-location consumption, but its source explicitly excludes
  direct global-symbol base-plus-offset authority, so these rows need distinct
  ownership unless refreshed diagnostics prove they now match idea 621.
- Split unsupported 16-byte/F128 local-memory width rows separately:
  `src/20010605-2.c`, `src/20040208-1.c`, and `src/ieee/inf-1.c`;
  `src/complex-7.c` should stay guarded because it mixes global-symbol and
  16-byte evidence.
- Keep sret/byval or aggregate pointer stack-home local-memory rows out of 614
  unless a later producer/policy route publishes explicit stack-home authority
  for local-memory use.
- Split large selected pointer-offset rows separately, represented by
  `src/ipa-sra-2.c` and `src/pr60822.c`; do not fold large-offset materializing
  policy into the narrow local-memory consumer route.

## Suggested Next

Hand off to the plan owner to close or retire idea 614 and record the residual
splits. If the plan owner keeps 614 open, the next packet should first name a
specific complete-authority local-memory consumer family with broader evidence
than the residuals above; none was found in this Step 4 classification.

## Watchouts

- The current object route already has focused pointer-value local-memory tests
  and helpers. Treat rows still failing here as missing adjacent policy or
  excluded selected-authority shapes unless a later packet proves a broad,
  complete-authority consumer family.
- Do not fold string constants, global symbols, sret/byval stack homes,
  aggregate homes, large offsets, unsupported 16-byte widths, BIR producer
  repair, direct pointer arithmetic policy, ABI, branch/select, runtime,
  expectation, unsupported marker, allowlist, timeout, or accounting work into
  this RV64 consumer route.
- `src/complex-7.c` has both global-symbol rows and 16-byte local-memory width
  evidence; keep it out of any narrow width-only claim.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.
