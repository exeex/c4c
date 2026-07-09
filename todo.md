Status: Active
Source Idea Path: ideas/open/621_rv64_prepared_global_value_location_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Prepared-Global Consumer Ownership

# Current Packet

## Just Finished

Step 1 refreshed prepared-global consumer ownership with diagnostic-only probes
for `src/pr36034-1.c`, `src/pr91137.c`, `src/ieee/20001122-1.c`, and
`src/991030-1.c`.

Current evidence:

- `src/pr36034-1.c`: still in scope for this prepared-global consumer route.
  Fresh RV64 object route fails with `unsupported_global_data: RV64 object route
  requires supported prepared global memory facts`. Prepared BIR has complete
  supported global-symbol memory facts for `x` and `tmp`: explicit symbol
  identity, byte offsets, 8-byte width, 8-byte alignment, byte-storage aggregate
  layout authority, and proven-in-bounds ranges. Value-location shape is FPR
  plus prepared FPR frame-slot homes for many loaded/stored aggregate lanes.
  First failing owner is RV64 prepared-global consumer, not producer authority.
- `src/pr91137.c`: still in scope for this prepared-global consumer route.
  Fresh RV64 object route fails with `unsupported_global_data: RV64 object route
  requires supported prepared global memory facts`. Prepared BIR has complete
  supported global-symbol memory facts for scalar `b` and aggregate globals
  `c`/`d`: explicit symbol identity, byte offsets, 4-byte width, 4-byte
  alignment, scalar or byte-storage aggregate layout authority, and
  proven-in-bounds ranges. Value-location shape is GPR plus prepared GPR
  frame-slot homes for large aggregate lanes. First failing owner is RV64
  prepared-global consumer, not producer authority.
- `src/ieee/20001122-1.c`: probed as a guard row only. Prepared BIR has
  complete scalar global-symbol facts for `p` and `a` with 8-byte accesses and
  FPR/GPR register value locations, but the fresh RV64 object route fails with
  `unsupported_global_data: RV64 object route supports only 1-, 2-, 4-, and
  8-byte prepared global memory accesses`. Keep this as a guard/policy row,
  not a Step 2/3 implementation driver.
- `src/991030-1.c`: probed as a guard row only. Fresh RV64 object route now
  succeeds. Prepared BIR shows a simple 8-byte scalar global load from `x`
  into FPR/GPR register homes. No Step 2/3 work should be derived from this
  row.

## Suggested Next

Proceed to Step 2 coverage for semantic prepared-global consumer contracts
covering the two in-scope shapes: FPR/frame-slot global double lanes from
`src/pr36034-1.c` and GPR/frame-slot global integer aggregate lanes from
`src/pr91137.c`.

## Watchouts

- Do not weaken prepared/global producer authority gates.
- Do not special-case named source files, global names, value ids, or final
  assembly shapes.
- Keep `src/ieee/20001122-1.c` guard-only unless the width-policy diagnostic is
  separately resolved; keep `src/991030-1.c` out of implementation packets
  because it already passes.
- Step 2/3 should target complete prepared/global facts consumed through
  prepared value locations. Avoid broad direct-global-symbol or producer
  reconstruction work.

## Proof

Proof command run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Result: passed. The build reported `ninja: no work to do`; CTest reported
`100% tests passed, 0 tests failed out of 347`. Proof log: `test_after.log`.
