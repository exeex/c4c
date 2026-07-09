Status: Active
Source Idea Path: ideas/open/641_aggregate_global_object_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh And Classify Aggregate Global Residuals

# Current Packet

## Just Finished

Step 1 refreshed the five representative aggregate/global residual rows with a
fresh RV64 gcc torture allowlist probe and prepared-BIR authority dumps.

Rows with a shared complete-authority aggregate/global shape:

- `src/complex-7.c`: first owner is byte-storage aggregate global-object lane
  materialization. Probe still fails with `unsupported_global_data: RV64
  object route supports only 1-, 2-, 4-, and 8-byte prepared global memory
  accesses`. Prepared evidence has explicit global symbols `f1..f5` lanes
  `[0,4)` and `[4,8)` width 4, `d1..d5` lanes `[0,8)` and `[8,16)` width 8,
  and `ld1..ld5` lanes `[0,16)` and `[16,32)` width 16. All selected global
  lanes use `layout_authority=byte_storage_aggregate`,
  `range_verdict=proven_in_bounds`, `base_plus_offset=yes`, and selected
  frame-slot destinations. The Step 2 candidate inside this row is the
  complete-authority 16-byte byte-storage aggregate global lane, not scalar
  direct-global local memory.
- `src/pr60822.c`: first owner is aggregate/global data materialization after
  the large selected pointer-offset route. Probe still fails with
  `unsupported_global_data: RV64 object route requires supported prepared
  global memory facts`. Prepared evidence has global object `x` stores at
  offsets `800000` and `1700004`, width 4, `layout_authority=
  byte_storage_aggregate`, `range_verdict=proven_in_bounds`,
  `base_plus_offset=yes`, and selected store-source freshness from same-block
  binary producers. The same dump also contains pointer-value reads through
  `%p.p` at offsets `800000` and `1700004` with `layout_authority=unknown` and
  `range_verdict=unknown_compatible`; those are adjacent large-offset facts,
  but the current first stop is global data.
- `src/pr88739.c`: prepared evidence contains complete byte-storage aggregate
  global reads from object `v` at offsets `12` and `14`, widths 4 and 2,
  `layout_authority=byte_storage_aggregate`, and
  `range_verdict=proven_in_bounds`. The actual first probe stop remains
  `unsupported_local_memory_access`; the blocking owner is local aggregate or
  frame-slot policy, because `foo` reads frame slot `#0` at offset `12` with
  width 4 under `range_verdict=proven_out_of_bounds`. Keep this row outside
  Step 2 unless the supervisor explicitly selects local aggregate/frame-slot
  policy instead of idea 641 global materialization.

Rows lacking complete authority for idea 641 or owned elsewhere:

- `src/pr49073.c`: first owner is not aggregate global-object materialization.
  Probe still fails with `unsupported_local_memory_access`, while prepared
  evidence shows a move-bundle destination fan-in shape:
  `authority=stack_destination_register_fan_in` at block index 1 instruction
  13, plus store-source rows with `status=missing_destination_access` and
  `source_producer=unknown` on join transfers. The row has complete global
  lanes for object `a` offsets `0..24` width 4 and scalar direct-global
  accesses to `c`, but the current blocker is prepared/prealloc
  destination/source authority, not idea 641.
- `src/pr60017.c`: first owner is call ABI / sret stack-home policy, not idea
  641. Probe fails with `unsupported_call_abi` for same-module call `main ->
  func`, `memory_return=%t0`, `memory_encoding=frame_slot`, `sret_arg_index=0`,
  `memory_size=16`, `memory_align=4`. Prepared evidence includes global object
  `x` aggregate loads at offsets `0,4,5,6,7,8,10,12,14`; only offset `0` has
  `layout_authority=byte_storage_aggregate`, while the remaining byte/halfword
  lanes have `layout_authority=unknown`. The sret pointer stores also have
  pointer-value `layout_authority=unknown`, so this row lacks complete idea 641
  authority and is owned by call ABI / aggregate stack-home policy.

## Suggested Next

Select Step 2 only if the supervisor wants the complete-authority
byte-storage aggregate global-object lane family. The narrowest candidate is
`src/complex-7.c` 16-byte `byte_storage_aggregate` global lanes with explicit
symbols, offsets, widths, proven ranges, and selected frame-slot destinations;
`src/pr60822.c` can be a boundary row for supported global memory facts at
large aggregate offsets. Do not include `src/pr49073.c` or `src/pr60017.c` in
that implementation packet, and keep `src/pr88739.c` as a boundary row unless
its local aggregate/frame-slot owner is split separately.

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

Ran the supervisor-selected proof/probe command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/641_step1_aggregate_global.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: build was up to date; allowlist probe completed with `total=5
passed=0 failed=5`, preserving `test_after.log`. Focused prepared-BIR
inspection artifacts were written under `build/agent_state/641_step1_*.prepared.txt`.
