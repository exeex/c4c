Status: Active
Source Idea Path: ideas/open/358_rv64_object_route_abi_width_edges.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Route Simple Call and FPR ABI Edges

# Current Packet

## Just Finished

Completed Plan Step 4 classification for `src/20030125-1.c` after the
producer-side RV64 FPR formal ABI identity slice.

The representative still does not reach first object instruction emission.
`--codegen obj` fails during object-route prepared-shape admission with:

```text
[RV64_C4C_OBJ_COMPILE_FAIL]
error: --codegen obj failed: RISC-V backend object route unsupported
prepared module shape: unsupported_param_home: RV64 object route requires
all parameters in supported GPR homes
```

Prepared BIR reaches the expected call/cast shape before that gate:
`@t`, `@q`, and `@q1` each take `float %p.a`, perform
`bir.fpext float %p.a to double`, call `sin`/`floor`, and then either
`bir.fptrunc` or return the double result. Their call plans use FPR
source/result banks, but the fixed formal homes still appear as FPR register
parameter homes that the object-route gate rejects:

```text
prepared.func @t
  home %p.a value_id=0 kind=register reg=a0
...
prepared.func @q
  home %p.a value_id=4 kind=register reg=a0
...
prepared.func @q1
  home %p.a value_id=8 kind=register reg=a0
...
prepared.func @floor
  home %p.a value_id=11 kind=register reg=a0
...
prepared.func @sin
  home %p.a value_id=14 kind=register reg=a0
```

The first boundary is therefore object-route admission/support for prepared
RV64 hard-float parameter homes, not an emitted `fpext`, `fptrunc`, call, or
return instruction.

## Suggested Next

Route prepared RV64 hard-float formal parameter homes through the object-route
shape gate using the published target register identity, then rerun
`src/20030125-1.c` to expose the first real instruction-lowering boundary.

## Watchouts

- Treat `src/20030125-1.c` as a representative only. The semantic class is
  prepared FPR cast/call ABI lowering, not testcase-specific math folding.
- The current failure is earlier than cast/call lowering: the object route still
  rejects FPR parameter homes with `unsupported_param_home` and the wording says
  only GPR homes are supported.
- The prepared dump prints FPR formal homes as `reg=a0`/`units=a0` even while
  `bank=fpr`; do not add string parsing of that spelling in object emission.
  The next slice should consume the prepared target-register identity or improve
  its publication/printing if the identity is absent at the object gate.
- Do not broaden the cast type matrix. The representative still only needs
  `FPExt F32 -> F64` and already-covered `FPTrunc F64 -> F32`.
- Preserve the object route's reliance on prepared register facts. Do not add
  fallback parsing of ABI register spellings like `fa0` in object emission.
- Floating calls, libm behavior, and stack-slot FPR materialization remain
  outside this classification packet.

## Proof

Delegated proof failed as the expected classification source:

```sh
tmp=$(mktemp); printf 'src/20030125-1.c\n' > "$tmp"; ALLOWLIST="$tmp" CASE_TIMEOUT_SEC=20 STOP_ON_FAILURE=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log; rc=$?; rm -f "$tmp"; exit $rc
```

Result: `src/20030125-1.c` failed at object compile with
`unsupported_param_home: RV64 object route requires all parameters in supported
GPR homes`.

Supplemental classification command:

```sh
./build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030125-1.c
```

Proof logs:
- `test_after.log`
