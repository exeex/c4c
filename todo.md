Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split Or Park The Next Lifecycle State

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by classifying the 24 post-345
`c_testsuite_aarch64_backend_*` residuals from `/workspaces/c4c/test_after.log`
against closed focused owners through 345. This packet was classification-only:
no implementation, test, expectation, unsupported, runner, timeout, proof-log,
or CTest-registration files were edited.

Inventory basis remained the Step 1 proof log:
`cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure > /workspaces/c4c/test_after.log 2>&1`

Stage/failure-family classification:

- Local backend/unit/CLI residuals: none. All 24 failures are external
  `c_testsuite_aarch64_backend_*`.
- Compile-stage semantic admission: `00005` fails before generated assembly
  with `[FRONTEND_FAIL]` in semantic `lir_to_bir` `store local-memory`. This is
  not closed idea 312's old `00204`/`00216` local-memory admission shape without
  fresh semantic-BIR evidence.
- Runtime nonzero, pointer/null/scalar condition result publication: `00020`,
  `00103`, `00112`, and `00200`. Generated `00020.c.s` and `00103.c.s` load
  through the pointer chain then return `x21`, the pointer home, not the loaded
  int; `00112.c.s` returns stale `x13` for `"abc" == NULL`; `00200` prints
  `0 test(s) failed` but exits nonzero, consistent with stale final condition
  result publication. These overlap filenames from idea 336, but the current
  first bad fact is not the old late epilogue overwrite.
- Runtime nonzero, FP comparison/expression value materialization: `00119`,
  `00123`, and `00174`. `00119.c.s`/`00123.c.s` load global double `x` and
  return raw `x13` bits instead of a boolean compare result; `00174.c.s`
  repeatedly prints through stale `d13`/uninitialized comparison homes. This
  remains outside closed idea 339's integer local sizing/writeback owner.
- Runtime nonzero/mismatch, direct-call argument and formal publication:
  `00140`, `00159`, `00170`, `00175`, and `00218`. Representative generated
  evidence: `00159.c.s` `myfunc` uses `w20` instead of incoming `w0`;
  `00175.c.s` calls/conversion paths use stale `w13`/`d13` for scalar and FP
  arguments; `00170.c.s` loses the 8th variadic/stack integer argument;
  `00140.c.s` passes uninitialized `x20` for the aggregate/address argument;
  `00218.c.s` calls `convert_like_real` with uninitialized `x21` instead of
  `&convs`. This is adjacent to closed idea 309's parked direct-call buckets
  and idea 311's old selected call-boundary printer failure, but the current
  evidence is runtime ABI argument/formal publication, not a machine-printer
  diagnostic.
- Runtime mismatch, addressable memory / indexed aggregate / pointer-local
  materialization: `00130`, `00176`, `00181`, `00182`, `00195`, `00205`, and
  `00216`. These show array, recursive/global array, switch/string-buffer,
  global aggregate, case-table, and aggregate initializer corruption. They are
  too broad to split as one owner without more generated-code probes. `00205`
  is not a reopen of closed ideas 302, 304, or 305: it no longer shows the old
  scalar operand, timeout, or high stack-home residual; it is now a broader
  table/aggregate materialization mismatch.
- Runtime mismatch, libc/file/string residual: `00186` and `00187`.
  `00186` only prints the first `sprintf` loop line; `00187` emits `h` where
  expected output starts with `hello`. These are parked until a probe ties them
  to call/vararg publication, stack buffer materialization, or libc IO state.
- Timeouts/output-storm: `00173` and `00207`. These remain separate from
  non-timeout runtime buckets. Do not change timeout policy or treat them as
  owner-ready without targeted generated-code/runtime evidence.

Closed-boundary comparison:

- Do not reopen ideas 301/302/304/305 from filename recurrence. The current
  residuals have advanced past the old store-printer, scalar operand, timeout,
  and `00205` high stack-home facts recorded in those closures.
- Do not reopen idea 312 for `00216`: current `00216` is a runtime mismatch
  with aggregate/global/local initialization corruption, not the old semantic
  local-memory admission diagnostic.
- Do not reopen ideas 336/337 from the remaining `00020`, `00103`, `00112`,
  `00159`, or `00170` counts. The current generated-code evidence points to
  pointer/result selection or call argument/formal publication, not the closed
  stale return-result overwrite or callee-saved scalar-home preservation facts.
- Do not reopen ideas 338 through 345. No current residual shows the old
  scalar-cast printer diagnostic, scalar local sizing/writeback first bad fact,
  Duff/fallthrough/loop-carried pointer sequence, or scalar-select result
  publication evidence.

Best next semantic owner candidate:

- Split a focused AArch64 direct-call argument/formal publication owner covering
  the current representative family `00140`, `00159`, `00170`, `00175`, and
  `00218`.
- The initial owner should localize how prepared call operands and callee
  formal homes become AAPCS64 ABI argument registers/stack slots for scalar,
  FP, address-of-local, by-value aggregate, and overflow/variadic-stack
  arguments.
- Acceptance should require generated-code evidence before repair and focused
  backend coverage, not just c-testsuite count improvement. The route should
  reject filename-specific fixes, single-register shortcuts such as `w13`,
  `w20`, or `x21`, and reopening closed idea 309/311 unless a printer
  diagnostic or indirect-call first bad fact returns.

## Suggested Next

Execute `plan.md` Step 3 through plan-owner authority: create and switch to a
focused AArch64 direct-call argument/formal publication idea for `00140`,
`00159`, `00170`, `00175`, and `00218`, or ask the plan owner to park the
classification if it wants additional generated-code probes before splitting.

## Watchouts

- This umbrella is classification-only; implementation should move to a
  focused split idea before code edits begin.
- The direct-call candidate is promising because it has multiple generated-code
  first bad facts, but it should not absorb pointer returns, FP comparison,
  aggregate/table corruption, libc/file IO, or timeout buckets without fresh
  evidence.
- `00170` should be treated as a stack/overflow argument representative, not a
  generic enum residual, based on the missing 8th printed value.
- `00218` is address-of-local direct-call argument preparation by current
  generated-code evidence; closed idea 309 explicitly parked that shape.
- Timeouts `00173` and `00207` were not rerun individually in this packet.

## Proof

Read-only classification proof used the existing delegated inventory log:
`cd /workspaces/c4c/build && ctest -j10 -R backend --output-on-failure > /workspaces/c4c/test_after.log 2>&1`

`test_after.log` was preserved and not overwritten. Additional read-only
inspection covered generated assembly under
`build/c_testsuite_aarch64_backend/src/*.c.s` and source tests under
`tests/c/external/c-testsuite/src/*.c`. No compile or CTest command was rerun
for this classification-only packet.
