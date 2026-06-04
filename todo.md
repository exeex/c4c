Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Trace The First Wrong ABI Fact

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: traced the first wrong ABI fact for the
AArch64 `00204.c` mismatch without implementation changes.

Owned repair layer: `src/backend/prealloc/call_plans.cpp`, specifically the
byval aggregate call-source/materialization path around
`select_byval_payload_lane_source`, `select_prepared_call_argument_source`, and
`plan_prepared_aggregate_transport`.

Last known-good upstream fact:

- Semantic and prepared BIR for `arg()` correctly load the global aggregate
  payload bytes before the direct byval calls. For example, `fa_s1(s1)` is
  represented as `%t2.global.aggregate.load.0 = bir.load_local i8 %t2.0, addr
  s1`, followed by `bir.store_local %t2.0, ...`, then `bir.call void fa_s1(ptr
  byval(size=1, align=1) %t2)`. `fa_s2(s2)` similarly has two
  `.global.aggregate.load.` byte loads from `s2` and `s2+1`.
- The call ABI classification itself is plausible for these first cases:
  prepared call plans place `fa_s1` and `fa_s2` byval aggregate payloads in
  `x0` with `reason=call_arg_byval_aggregate_register_lanes`. This is the
  expected AAPCS64 direct small-aggregate register path, not a wrong register
  number or a variadic-entry decision.

First wrong downstream fact:

- The prepared byval source selection fails to bind those `.global.aggregate.load.`
  payload bytes as the source lane. In the prepared artifact, `%t2` is treated
  as a register aggregate carrier in `x21`, `%t3` as a stack-slot carrier, and
  the call move then moves those carriers to `x0`; the actual global-byte
  loads are only transient homes in `x13`.
- The emitted assembly confirms the miss before the first direct aggregate
  call: after `printf("Arguments:\n")`, it uses `mov x13, x0` and stores
  `w13` into the `%t2`/`%t3` stack bytes before `bl fa_s1`/`bl fa_s2`, instead
  of loading from `s1`/`s2`. That explains the repeated low control-byte output
  and places the first wrong fact before AArch64 variadic lowering.

Why earlier layers are not currently blamed:

- Frontend/HIR-to-BIR lowering preserves the source intent as per-byte global
  aggregate loads feeding byval calls.
- General AArch64 call ABI register assignment is not the first wrong fact for
  `fa_s1`/`fa_s2`; the register target is `x0`, which is correct for these
  small integer-class byval aggregates.
- `call_moves.cpp` records the move that the call plan describes, but the move
  is already consuming the wrong aggregate carrier/source. The repair owner is
  the call-plan source selection/materialization layer that should connect
  `.global.aggregate.load.` lanes to the byval transport plan.
- `variadic_entry_plans.cpp` and `src/backend/mir/aarch64/codegen/variadic.cpp`
  still likely own later stdarg/HFA failures, but they are downstream of this
  first direct aggregate corruption and should not be repaired first.

Concrete source-side clue:

- `select_byval_payload_lane_load_source` currently recognizes
  `.array.aggregate.load.` suffixes when searching for contiguous byval lane
  stores before a call. The `00204.c` global struct path emits
  `.global.aggregate.load.` suffixes, so the direct global aggregate payload
  source is missed before the byval register-lane transport is planned.

## Suggested Next

Proceed to Step 3: add focused contract coverage for the byval aggregate
source-selection fact. The smallest useful test should prove that an AArch64
small byval aggregate argument whose payload is assembled from same-module
global aggregate byte loads gets a prepared byval register-lane source/transport
from those payload bytes, not from a stale aggregate carrier register or prior
call result.

## Watchouts

- Do not downgrade `00204.c` expectations or mark it unsupported.
- Do not special-case `00204.c` or its literal output shape.
- Keep `00032.c` and `00182.c` visible as AArch64 guard cases.
- Treat narrow probes as ABI-fact probes, not testcase-shaped shortcuts.
- The mismatch is broader than the final `stdarg:` section: direct aggregate
  argument and return paths are visibly wrong too, while `MOVI:` remains green.
- This Step 2 trace identifies the first wrong layer for the direct aggregate
  path only. String/integer varargs and HFA varargs remain separately corrupt,
  but should be revisited after the earlier byval aggregate source repair is
  tested and implemented.
- The next test must target the semantic source-selection rule, not the full
  `00204.c` output.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8, expected for this trace packet because no implementation
change was allowed and `00204.c` remains the known failing target. Guard status:
`c_testsuite_aarch64_backend_src_00032_c` passed and
`c_testsuite_aarch64_backend_src_00182_c` passed;
`c_testsuite_aarch64_backend_src_00204_c` failed with runtime mismatch.
Canonical executor proof log: `test_after.log`.
