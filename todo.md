Status: Active
Source Idea Path: ideas/open/641_aggregate_global_object_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select One Shared Authority Family

# Current Packet

## Just Finished

Step 2 selected exactly one implementation family from the committed Step 1
evidence: complete-authority 16-byte byte-storage aggregate global-object lane
materialization for explicit global symbols, anchored on `src/complex-7.c`
`ld1..ld5` lanes at offsets `0` and `16`.

Positive authority shape:

- global-symbol base with explicit symbol identity
- 16-byte lane width and 16-byte alignment
- `base_plus_offset=yes`
- `layout_authority=byte_storage_aggregate`
- `range_verdict=proven_in_bounds`
- selected destination facts already present for the copied frame-slot lanes

Likely missing repair owner: RV64 consumer policy. Prepared evidence already
contains the selected global memory facts, but the object route still rejects
`src/complex-7.c` with `unsupported_global_data: RV64 object route supports
only 1-, 2-, 4-, and 8-byte prepared global memory accesses`. Step 3 should
teach the RV64 global-data consumer to materialize this complete 16-byte
byte-storage aggregate global lane by composing legal memory operations, while
preserving the existing fact requirements.

Explicit negative cases that must remain fail-closed:

- `src/pr60822.c`: boundary row only. Its global object `x` stores at large
  offsets have complete 4-byte aggregate facts, but the row also has adjacent
  pointer-value reads with `layout_authority=unknown`; do not fold large
  selected pointer-offset policy into this packet.
- `src/pr88739.c`: boundary row only. It contains complete global reads from
  object `v`, but its current first stop is `unsupported_local_memory_access`
  from local/frame-slot policy with an out-of-bounds frame-slot read.
- `src/pr49073.c`: fail-closed as prepared/prealloc destination-source fan-in
  ownership. It has aggregate global lanes, but the current blocker is
  `stack_destination_register_fan_in` plus missing destination/source facts.
- `src/pr60017.c`: fail-closed as call ABI / sret stack-home ownership. It
  has incomplete aggregate global authority beyond offset `0` and the first
  stop is `unsupported_call_abi`.
- Any global lane missing explicit object identity, byte range, offset, extent,
  selected destination, `byte_storage_aggregate` authority, or proven in-bounds
  range must continue to reject with a precise diagnostic.

## Suggested Next

Step 3 implementation packet: update only the RV64 global-data consumer for
complete 16-byte `byte_storage_aggregate` global-symbol accesses with proven
in-bounds facts, using `src/complex-7.c` as the positive probe and preserving
fail-closed diagnostics for `src/pr60822.c`, `src/pr88739.c`, `src/pr49073.c`,
and `src/pr60017.c`.

## Watchouts

- Do not treat aggregate global-object materialization as scalar direct
  `addr @symbol` local-memory support.
- Do not add testcase-shaped handling for the named rows or specific
  aggregate offsets.
- Do not infer aggregate lanes, byte ranges, or destination authority from
  final assembly layout or source object spelling.
- `src/pr49073.c` has aggregate global lanes in the dump, but its first owner
  is destination/source fan-in authority. Counting it as idea 641 progress
  would be route drift.
- `src/pr60017.c` and `src/pr88739.c` have missing or contradicted local/sret
  layout authority. They should not be used to justify RV64 aggregate-global
  materialization.

## Proof

No new proof was required for this evidence-only Step 2 packet. Preserved the
committed Step 1 proof/probe log at `test_after.log`:

`cmake --build --preset default && ALLOWLIST=build/agent_state/641_step1_aggregate_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build was up to date; allowlist probe completed with `total=5
passed=0 failed=5`, preserving `test_after.log`. Focused prepared-BIR
inspection artifacts were written under `build/agent_state/641_step1_*.prepared.txt`.
