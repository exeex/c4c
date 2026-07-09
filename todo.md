Status: Active
Source Idea Path: ideas/open/614_rv64_pointer_local_memory_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Split Or Close-Readiness Classification

# Current Packet

## Just Finished

Executed Step 3, `Broaden Within Selected Pointer Authority`, as a residual
refresh after the frame-slot local-memory consumer slice. No implementation
files were changed.

Refresh command and artifacts:

- Candidate set: the `35` prior `unsupported_local_memory_access` rows from
  `build/agent_state/612_step3_failure_reason_rows.tsv`.
- Probe command:
  `ALLOWLIST=build/agent_state/614_step3_residual_refresh/local_memory_candidates.allowlist STOP_ON_FAILURE=0 VERBOSE_FAILURES=0 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/614_step3_residual_refresh/probe.log 2>&1 || true`
- Result: `0/35` passed; all `35` still stop at
  `unsupported_local_memory_access`.

Current first diagnostic split:

- `32` rows: `unsupported_local_memory_access: RV64 object route requires
  prepared frame-slot or pointer-value base-plus-offset local memory
  addressing`.
- `3` rows: `unsupported_local_memory_access: RV64 object route supports only
  1-, 2-, 4-, and 8-byte prepared local memory accesses`
  (`src/20010605-2.c`, `src/20040208-1.c`, `src/ieee/inf-1.c`).

Prepared-addressing split from focused dumps:

- String-constant local-memory base rows:
  `src/20000722-1.c`, `src/20010123-1.c`, `src/20011109-2.c`,
  `src/20021204-1.c`, `src/20030920-1.c`, `src/920429-1.c`,
  `src/930429-1.c`, `src/pr34415.c`, `src/pr35800.c`, and
  `src/ptr-arith-1.c`.
- Global-symbol local-memory base rows:
  `src/20021204-1.c`, `src/920429-1.c`, `src/921117-1.c`,
  `src/complex-7.c`, `src/pr46309.c`, `src/pr49073.c`,
  `src/pr57861.c`, `src/pr58431.c`, `src/pr58984.c`,
  `src/pr60017.c`, `src/pr60822.c`, `src/pr66556.c`,
  `src/pr68185.c`, `src/pr68321.c`, `src/pr70005.c`,
  `src/pr88739.c`, and `src/struct-ret-1.c`.
- Pointer-value-only supported-width rows:
  `src/20020215-1.c`, `src/950628-1.c`, `src/ipa-sra-2.c`,
  `src/pr30185.c`, `src/pr38969.c`, and `src/pr52129.c`. These do not form a
  clean Step 3 implementation packet: the stack-home cases are sret/byval or
  aggregate homes deliberately excluded by the current RV64 helper, and the
  register-base case includes a large selected offset beyond the current
  immediate/base-offset consumer shape.
- Frame-slot-only supported-width outlier:
  `src/941110-1.c`; prepared facts show selected frame-slot accesses only, but
  the remaining rejection is not a broad adjacent pointer-value family.

## Suggested Next

Do not implement a Step 3 consumer from this refresh. Hand off to plan owner or
supervisor for Step 4 residual split/close-readiness classification. The
durable split candidates are string-constant local-memory policy, global-symbol
local-memory policy, unsupported 16-byte/F128 local-memory width, sret/byval or
aggregate pointer stack-home policy, and large selected pointer offsets.

## Watchouts

- The current object route already has focused pointer-value local-memory tests
  and helpers. Treat rows still failing here as missing adjacent policy or
  excluded selected-authority shapes unless a later packet proves a broad,
  complete-authority consumer family.
- Do not fold string constants, global symbols, sret/byval stack homes, large
  offsets, unsupported 16-byte widths, BIR producer repair, direct pointer
  arithmetic policy, ABI, branch/select, runtime, expectation, unsupported
  marker, allowlist, timeout, or accounting work into this RV64 consumer route.
- `src/complex-7.c` has both global-symbol rows and 16-byte local-memory width
  evidence; keep it out of any narrow width-only claim.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed; `test_after.log` reports `100% tests passed, 0 tests failed
out of 346`.
