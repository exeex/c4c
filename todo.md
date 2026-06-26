Status: Active
Source Idea Path: ideas/open/405_prepared_local_aggregate_slot_layout_facts.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Contradictory Local Aggregate Slot Facts

# Current Packet

## Just Finished

Step 1 classification completed for `tests/c/external/gcc_torture/src/20020225-2.c`
without implementation or expectation changes.

Representative facts from
`build/agent_state/405_step1_dumps/20020225-2.prepared-bir.txt`:

- Semantic BIR uses one local union address name for the overlapping accesses:
  `bir.store_local %lv.a.0, double %t1, addr %lv.a.0`,
  `bir.store_local %lv.a.0, i32 1, addr %lv.a.0`, and
  `%t4 = bir.load_local i32 %t4.addr, addr %lv.a.0`.
- Prepared stack layout publishes the union storage as eight separate
  one-byte local slots instead of one covering union extent: object `#0`
  `%lv.a.0` has `type=i8 size=1 align=8 address_exposed=yes
  requires_home_slot=yes permanent_home_slot=yes`, slot `#0` has
  `offset=0 size=1 align=8 fixed_location=yes`, and sibling slots `#1..#7`
  cover byte offsets `1..7` individually with `size=1 align=8`.
- The selected frame slot for every access is slot `#0`, so its valid byte
  range is `[0, 1)`. The surrounding fixed frame is `frame_size=16` and
  `frame_alignment=8`; scratch `%t4.addr` is separate slot `#8` at
  `offset=8 size=4 align=4`.
- Rejected prepared-addressing facts are:
  `inst_index=1` store of `%t1`, `frame_slot=#0 offset=0 size=8 align=8`,
  range `[0, 8)`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`;
  `inst_index=2` i32 store, `frame_slot=#0 offset=0 size=4 align=4`,
  range `[0, 4)`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`;
  `inst_index=3` i32 load result `%t4`, `frame_slot=#0 offset=0 size=4
  align=4`, range `[0, 4)`, `layout_authority=unknown`,
  `range_verdict=proven_out_of_bounds`.

Classification: producer-side local union extent publication is missing. The
producer byte-slices the local union into one-byte frame slots and then
prepared addressing selects the first byte slot as if it covered the whole
overlaid union storage. This is not an RV64 object-emission recovery problem;
the next repair should publish or select a prepared local aggregate/union
storage extent that covers the active member access ranges while preserving
overlay/member offsets. Treat this as missing aggregate/union extent
publication with incorrect byte-slot selection as the symptom, not as a
filename-specific field-slot special case.

Nearby same-family candidate found:
`tests/c/external/gcc_torture/src/ieee/mul-subnormal-single-1.c`.
The `u2f` and `f2u` prepared dumps in
`build/agent_state/405_step1_dumps/ieee_mul_subnormal_u2f.prepared-bir.txt`
and
`build/agent_state/405_step1_dumps/ieee_mul_subnormal_f2u.prepared-bir.txt`
show local `union uf { unsigned int u; float f; }` storage published as
one-byte `%lv.u.0..3` slots. In `u2f`, slot `#0` is `offset=0 size=1
align=4` but both the i32 store and float load at offset 0 are size 4 with
`range_verdict=proven_out_of_bounds`; in `f2u`, slot `#5` has the same
one-byte facts and both the float store and i32 load are rejected as size 4.
The candidate torture run also fails first with
`unsupported_local_memory_access`.

## Suggested Next

Delegate Step 2 to repair the producer path that creates fixed frame-slot
metadata for byte-sliced local aggregate/union storage. The narrow target is
the prepared stack-layout/addressing producer that turns one source local union
object into `%lv.<name>.0..N` one-byte local slots and then records local
memory accesses against only byte slot zero. The proof should start with:
`cmake --build --preset default && mkdir -p build/agent_state/405_step2_dumps`
plus prepared dumps for `src/20020225-2.c` and
`src/ieee/mul-subnormal-single-1.c`, then an allowlisted
`scripts/check_progress_rv64_gcc_c_torture_backend.sh` run covering both
cases, and finally `ctest --test-dir build -j --output-on-failure -R
'^backend_' > test_after.log`.

## Watchouts

- Do not repair this by fabricating slot size, source union layout, aliasing,
  or containment facts in RV64 object emission.
- Do not special-case `20020225-2.c`.
- Do not simply widen every one-byte local slot. The producer must preserve
  byte/member offsets and overlay semantics while publishing an explicit
  covering aggregate/union extent or selecting an existing covering slot for
  wide member accesses.
- `src/ieee/minuszero.c` and `src/991228-1.c` look source-adjacent but do not
  currently reach prepared BIR for the relevant function; both fail earlier in
  semantic local-memory lowering during this packet's candidate scan.
- Split distinct broad stack-frame, parameter-home, or non-local-aggregate
  producer defects instead of absorbing them here.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && mkdir -p build/agent_state/405_step1_dumps
&& printf '%s\n' 'src/20020225-2.c' >
build/agent_state/405_step1_local_aggregate.allowlist.txt && { ALLOWLIST=build/agent_state/405_step1_local_aggregate.allowlist.txt
STOP_ON_FAILURE=0 VERBOSE_FAILURES=1
scripts/check_progress_rv64_gcc_c_torture_backend.sh >
build/agent_state/405_step1_local_aggregate_probe.log 2>&1 || true; } && rg -n
'unsupported_local_memory_access|unsupported_instruction_fragment|unsupported
RV64 object lowering|\[rv64-gcc-torture\]|pass\t|fail\t|RV64_C4C_OBJ|error:'
build/agent_state/405_step1_local_aggregate_probe.log && ctest --test-dir
build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: representative probe reproduced
`fail src/20020225-2.c` with `RV64_C4C_OBJ_COMPILE_FAIL` and
`unsupported_local_memory_access`; backend CTest subset completed with
`test_after.log` as the proof log.
