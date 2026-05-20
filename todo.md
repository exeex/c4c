Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Failure Families

# Current Packet

## Just Finished

Step 2 classified the current backend-regex failure families from
`test_after.log` and read-only generated artifacts.

Current first-pass owner buckets:

- **Compile/printer scalar machine-node operand facts** (`2`):
  `c_testsuite_aarch64_backend_src_00164_c` and
  `c_testsuite_aarch64_backend_src_00214_c`.
  - `00164.c` fails before runtime with
    `cannot print AArch64 machine node family=scalar opcode=mul: scalar
    mul/div/rem node has incomplete printable rhs facts`. Source line shape is
    scalar arithmetic around `a + b * c / f`; prepared BIR contains
    `%t159 = bir.mul i32 %t157, %t158` and `%t168 = bir.mul i32 %t166, %t167`,
    with late values assigned to spill/frame slots such as `%t158` at
    `stack232`, `%t159` at `stack236`, `%t167` at `stack244`, and `%t168` at
    `stack248`.
  - `00214.c` fails before runtime with
    `cannot print AArch64 machine node family=scalar opcode=add: scalar
    add/sub/bitwise memory operands require prepared frame-slot sources`.
    The representative source uses pointer/integer arithmetic in `extend_brk`
    and related helper blocks.
  - Best current focused owner candidate: split a new focused idea for AArch64
    scalar machine-node operand fact preservation/printing for scalar
    arithmetic with spilled or memory operands. This is not an implementation
    packet; plan-owner should create or select the focused source idea first.

- **Fresh adjacent frame-allocation evidence, not yet the CTest first owner**:
  current partial generated artifacts for the compile/printer cases show
  frame-size/prologue tension that may be exposed after scalar printer
  localization:
  - `00164.c`: prepared summary for `main` reports `frame_size=256`, but the
    emitted partial assembly prologue reserves `sub sp, sp, #64` while later
    accesses include `[sp, #140]`, `[sp, #160]`, `[sp, #236]`, and
    `[sp, #252]`.
  - `00214.c`: prepared summary for `extend_brk` reports `frame_size=96`, but
    emitted partial assembly reserves `sub sp, sp, #48` while later accesses
    include `[sp, #64]`, `[sp, #72]`, `[sp, #80]`, and `[sp, #88]`.
  - This is fresh evidence relevant to parked idea 316, but the current CTest
    first bad facts for these tests are printer diagnostics. Reactivate idea
    316 only if a lifecycle/localization packet decides the frame allocation
    mismatch is the current first owner after or within the printer route.

- **Runtime mismatch: stack/local array and indexed-memory publication
  candidates** (`00157`, `00176`, `00185`): representative outputs show
  correct early stores followed by garbage or uninitialized-looking reloads.
  `00157.c` expected squares `1..100` but prints zeros and large negative
  values; prepared summary for `main` reports `frame_size=204` and emitted
  assembly reserves `#240`, so this is not the stale idea 316 small-prologue
  shape. `00185.c` array initializer output similarly needs generated-code
  localization before a focused owner is ready.

- **Runtime mismatch: loop/control-flow/recursion/pointer progression
  candidates** (`00168`, `00169`, `00170`, `00171`, `00182`, `00186`):
  examples include `00168.c` factorial printing all `1`s, `00169.c` nested
  loop output corrupting the middle induction value, `00170.c` enum/int output
  ending with `4094787984`, and `00171.c` printing `b is NULL` where `b is
  not NULL` is expected. These need focused generated-artifact localization.

- **Runtime mismatch: floating/math or variadic publication candidates**
  (`00174`, `00175`, `00195`): `00174.c` float/double math starts with
  `0.000000` values instead of `69.120003` and related results; `00175.c`
  corrupts later char/int/float vararg-style outputs; `00195.c` prints
  `0.000000, 0.000000` instead of `12.340000, 56.780000`. These are outside
  the parked `00204.c` HFA/byval/stdarg ideas unless fresh evidence ties them
  to the same owner.

- **Runtime mismatch: aggregate/global initializer and bitfield projection
  candidates** (`00205`, `00216`, `00218`): `00205.c` J-interpreter aggregate
  constants diverge, `00216.c` current mismatch is in broader aggregate
  initializer output rather than the stale `test_correct_filling` frame
  evidence, and `00218.c` prints `unsigned enum bit-fields broken`. These need
  separate localization before splitting.

- **Runtime nonzero opaque exit-code bucket** (`32`): most cases return silent
  numeric exits such as `1`, `16`, `80`, `96`, `128`, `144`, `192`, `208`,
  `240`, or `252`. Three current segfaulting representatives are
  `00140.c`, `00181.c`, and `00208.c`. This bucket is not ready for a focused
  owner without generated-code localization.

- **Timeout quarantine** (`4`): `00173.c`, `00187.c`, `00207.c`, and
  `00220.c` hit the 5-second CTest timeout. Keep these separate from ordinary
  runtime mismatch/nonzero families until a timeout-safe packet localizes
  whether they are loops, output storms, blocking runtime behavior, or harness
  artifacts.

Comparison against open/parked ideas:

- `ideas/open/316_aarch64_frame_slot_layout_consistency.md` is parked because
  its `00216.c` evidence was stale. Current `00164.c` / `00214.c` partial
  artifacts provide fresh adjacent frame-layout evidence, but not enough in
  this packet to override the scalar printer first bad facts.
- `ideas/open/326_aarch64_variadic_hfa_floating_residual.md`,
  `ideas/open/328_aarch64_byval_aggregate_call_argument_lane_publication.md`,
  `ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md`,
  `ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md`, and
  `ideas/open/332_aarch64_movi_zero_extension_materialization.md` are not
  current activation targets from this inventory. `00204.c` is green in the
  fresh backend-regex run, so those parked/scope-satisfied `00204.c` routes
  should not be reopened from this evidence.
- `ideas/open/327_aarch64_fixed_formal_entry_publication.md` also does not
  match the current first-pass evidence; no current failure was localized to a
  fixed-formal entry publication gap.

Recommended next lifecycle action: Step 3 should split a focused source idea
for the scalar machine-node operand fact/printer family represented by
`00164.c` and `00214.c`, or have plan-owner explicitly decide whether the
fresh partial frame evidence is strong enough to reactivate idea 316 before
that split. Do not implement under umbrella idea 295.

Prior Step 1 inventory captured the backend-regex log into `test_after.log`
with:

```bash
ctest --test-dir build -j --output-on-failure -R 'backend' > test_after.log 2>&1
```

Current CTest counts:

- selected: `354`
- passed: `299`
- failed total: `55`
- ordinary failed status: `51`
- timeout: `4`
- skipped/not run: `0`
- incomplete: `0`
- total real time: `5.58 sec`

First-pass split:

- local backend/unit/CLI and other non-c-testsuite backend-regex tests:
  `142` selected, `142` passed, `0` failed
- `c_testsuite_aarch64_backend_*`: `212` selected, `157` passed, `55` failed

Failure-mode buckets from `test_after.log`:

- `FRONTEND_FAIL` (`2`): `c_testsuite_aarch64_backend_src_00164_c`,
  `c_testsuite_aarch64_backend_src_00214_c`
- `RUNTIME_MISMATCH` (`17`):
  `c_testsuite_aarch64_backend_src_00157_c`,
  `c_testsuite_aarch64_backend_src_00158_c`,
  `c_testsuite_aarch64_backend_src_00159_c`,
  `c_testsuite_aarch64_backend_src_00168_c`,
  `c_testsuite_aarch64_backend_src_00169_c`,
  `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00171_c`,
  `c_testsuite_aarch64_backend_src_00174_c`,
  `c_testsuite_aarch64_backend_src_00175_c`,
  `c_testsuite_aarch64_backend_src_00176_c`,
  `c_testsuite_aarch64_backend_src_00182_c`,
  `c_testsuite_aarch64_backend_src_00185_c`,
  `c_testsuite_aarch64_backend_src_00186_c`,
  `c_testsuite_aarch64_backend_src_00195_c`,
  `c_testsuite_aarch64_backend_src_00205_c`,
  `c_testsuite_aarch64_backend_src_00216_c`,
  `c_testsuite_aarch64_backend_src_00218_c`
- `RUNTIME_NONZERO` (`32`):
  `c_testsuite_aarch64_backend_src_00004_c`,
  `c_testsuite_aarch64_backend_src_00011_c`,
  `c_testsuite_aarch64_backend_src_00013_c`,
  `c_testsuite_aarch64_backend_src_00014_c`,
  `c_testsuite_aarch64_backend_src_00015_c`,
  `c_testsuite_aarch64_backend_src_00016_c`,
  `c_testsuite_aarch64_backend_src_00018_c`,
  `c_testsuite_aarch64_backend_src_00019_c`,
  `c_testsuite_aarch64_backend_src_00020_c`,
  `c_testsuite_aarch64_backend_src_00022_c`,
  `c_testsuite_aarch64_backend_src_00052_c`,
  `c_testsuite_aarch64_backend_src_00077_c`,
  `c_testsuite_aarch64_backend_src_00086_c`,
  `c_testsuite_aarch64_backend_src_00087_c`,
  `c_testsuite_aarch64_backend_src_00089_c`,
  `c_testsuite_aarch64_backend_src_00103_c`,
  `c_testsuite_aarch64_backend_src_00111_c`,
  `c_testsuite_aarch64_backend_src_00112_c`,
  `c_testsuite_aarch64_backend_src_00116_c`,
  `c_testsuite_aarch64_backend_src_00117_c`,
  `c_testsuite_aarch64_backend_src_00118_c`,
  `c_testsuite_aarch64_backend_src_00119_c`,
  `c_testsuite_aarch64_backend_src_00121_c`,
  `c_testsuite_aarch64_backend_src_00123_c`,
  `c_testsuite_aarch64_backend_src_00124_c`,
  `c_testsuite_aarch64_backend_src_00139_c`,
  `c_testsuite_aarch64_backend_src_00140_c`,
  `c_testsuite_aarch64_backend_src_00144_c`,
  `c_testsuite_aarch64_backend_src_00153_c`,
  `c_testsuite_aarch64_backend_src_00181_c`,
  `c_testsuite_aarch64_backend_src_00200_c`,
  `c_testsuite_aarch64_backend_src_00208_c`
- `Timeout` (`4`): `c_testsuite_aarch64_backend_src_00173_c`,
  `c_testsuite_aarch64_backend_src_00187_c`,
  `c_testsuite_aarch64_backend_src_00207_c`,
  `c_testsuite_aarch64_backend_src_00220_c`

Notable exact nonzero exits visible in this first-pass capture:

- `c_testsuite_aarch64_backend_src_00140_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`
- `c_testsuite_aarch64_backend_src_00181_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`
- `c_testsuite_aarch64_backend_src_00208_c`: `RUNTIME_NONZERO`, exit
  `Segmentation fault`

## Suggested Next

Return to plan-owner for Step 3. The recommended lifecycle packet is to create
or select one focused source idea for AArch64 scalar machine-node operand
fact/printer support around `00164.c` scalar `mul` RHS facts and `00214.c`
scalar `add` memory/frame-slot operand facts. If plan-owner prefers
reactivating parked idea 316 from the fresh partial frame evidence, require a
short localization packet that proves frame allocation is the current first
owner rather than a downstream artifact of the printer failure.

## Watchouts

Do not edit implementation files or tests under this umbrella idea. Do not
reactivate parked ideas 316, 328, 329, or 332 from stale lifecycle notes; require
fresh generated-code, diagnostic, or proof evidence that their exact owner has
returned. Keep the unrelated transient
`review/326_stdarg_byval_route_review.md` untouched.

This packet did not perform deep owner classification and did not create or
modify source ideas. The first-pass inventory shows no local backend/unit/CLI
failures selected by the backend regex; all current failures are external
AArch64 c-testsuite backend tests.

The compile/printer pair is intentionally preferred as the next split because
it is a current compile-stage first bad fact with exact diagnostics. Runtime
families remain classified only at first-pass granularity; they need
representative generated-code localization before any repair idea is split.

## Proof

Read-only classification only. Inspected `test_after.log`, current generated
assembly under `build/c_testsuite_aarch64_backend/src/`, and focused prepared
BIR dumps for representative failures including `00164.c`, `00214.c`,
`00157.c`, `00168.c`, `00169.c`, `00174.c`, `00185.c`, and `00216.c`.
Did not overwrite `test_after.log` and did not run broad tests.

Step 1 previously ran the delegated proof command exactly:

```bash
ctest --test-dir build -j --output-on-failure -R 'backend' > test_after.log 2>&1
```

CTest exited nonzero because the inventory contains expected failing tests.
`test_after.log` is the canonical proof log for this packet.
