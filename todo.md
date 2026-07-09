Status: Active
Source Idea Path: ideas/open/649_pointer_global_local_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Representative Pointer/Global Local Evidence

# Current Packet

## Just Finished

Completed Step 1 diagnostics for
`tests/c/external/gcc_torture/src/pr57861.c`. The first current owner remains
pointer/global local publication, with the sharper boundary at RV64 object-route
admission for `%lv.l`: prepared BIR publishes global address `@f` into the local
pointer slot, but the frame-slot accesses for that local pointer slot still have
`layout_authority=unknown`.

Named evidence:

- Prepared pointer value/global object source: `@f`, global object `f`, with
  prepared home `value_id=15 kind=register reg=s1`.
- Local-slot destination: `%lv.l`, `object #1`, `frame_slot=#0`, offset 0,
  size 8, align 8.
- Publication point: semantic `bir.store_local %lv.l, ptr @f`; prepared
  `store_source function=foo block=block_3 inst=0 source=@f status=available
  intent=store_local_publication`.
- Load/use point: semantic `%t33 = bir.load_local ptr %lv.l`; prepared
  `access block=logic.end.27 inst_index=3 base=frame_slot result=%t33
  frame_slot=#0 offset=0 size=8 align=8 base_plus_offset=yes
  layout_authority=unknown range_verdict=proven_in_bounds`.
- Final source-level `*l = 0` path is currently represented as direct global
  `bir.store_global @f, i16 0`, whose prepared direct-global memory access has
  `layout_authority=scalar_layout`.

Available authority: direct global address materialization exists for `@f`
(`address_materialization block=block_3 inst_index=0 kind=direct_global
result=@f symbol=f policy=direct`), `%lv.l` frame-slot accesses are
base-plus-offset and proven in bounds, scalar frame-slot memory such as `%lv.k`
has `layout_authority=scalar_layout`, and direct global-symbol memory such as
`@f` also has `layout_authority=scalar_layout`.

Missing authority: `%lv.l` has no pointer/global local-publication authority
for its frame-slot store/load; both relevant prepared local-slot accesses
(`block_3 inst_index=0 stored=@f` and `logic.end.27 inst_index=3 result=%t33`)
remain `layout_authority=unknown`. This is distinct from scalar frame-slot
local-memory and from direct global-symbol local-memory, both of which already
have scalar layout authority in the same function.

## Suggested Next

Proceed to Step 2 by locating the producer/consumer boundary that should either
publish pointer/global local-publication authority for `%lv.l` or teach the RV64
object-route consumer to accept the existing direct-global publication fact plus
proven in-bounds frame-slot access.

## Watchouts

- Do not treat scalar frame-slot local-memory facts from idea 640 as
  pointer/global local-publication authority.
- Do not reopen direct global-symbol local-memory support from idea 631 or
  generic pointer freshness from idea 600 unless fresh evidence proves a new
  local-publication boundary.
- Do not use the `main` call-argument direct-global select-chain evidence as
  the owner for this idea; the representative owner is inside `foo` around
  `%lv.l`.
- Do not infer authority from source spelling, final assembly order,
  diagnostics, testcase identity, local/global names, or stack-slot shape.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No CTest proof was required and `test_after.log` was not overwritten.
Diagnostics and captured outputs are under
`build/agent_state/649_step1_pointer_global_local_evidence/`, with summary at
`build/agent_state/649_step1_pointer_global_local_evidence/summary.md`.

Commands captured:

- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-bir tests/c/external/gcc_torture/src/pr57861.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-prepared-bir tests/c/external/gcc_torture/src/pr57861.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-mir --mir-focus-function foo
  tests/c/external/gcc_torture/src/pr57861.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --trace-mir --mir-focus-function foo
  tests/c/external/gcc_torture/src/pr57861.c`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --codegen asm tests/c/external/gcc_torture/src/pr57861.c -o
  build/agent_state/649_step1_pointer_global_local_evidence/pr57861.s`
- `build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --codegen obj tests/c/external/gcc_torture/src/pr57861.c -o
  build/agent_state/649_step1_pointer_global_local_evidence/pr57861.o`

Return codes: `dump_bir=0`, `dump_prepared_bir=0`, `dump_mir_foo=0`,
`trace_mir_foo=0`, `codegen_asm=1`, `codegen_obj=2`. Object route failed with
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing`.
