Status: Active
Source Idea Path: ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Runtime Publication Mismatch

# Current Packet

## Just Finished

Step 1 audit identified the first incorrect runtime publication shape in
`src/20000112-1.c`: a `select_materialization`/phi-edge join publication at
`special_format` `logic.end.7`, where predecessor terminators must publish the
chosen incoming value into the join result home before the join block consumes
it.

Required semantic operands and homes:
- Join result `%t13`/value_id 5 has register home `t0`.
- Skip incoming `logic.skip.5 -> logic.end.7` is immediate `1` and must be
  materialized into `%t13`'s home before entering the join.
- RHS incoming `logic.rhs.end.6 -> logic.end.7` is `%t9`/value_id 4, also in
  `t0`, after booleanizing `%t8`.
- The RHS call result `%t8`/value_id 3 has register home `s2`; it is not the
  join result and is undefined/stale on the skip edge.

Prepared evidence names this as `join_transfer logic.end.7 result=%t13
kind=phi_edge carrier=select_materialization ownership=authoritative_branch_pair`
with `parallel_copy logic.skip.5 -> logic.end.7` moving `1 -> %t13` at the
predecessor terminator. The object route emits `li t0,1` in the skip block, but
the join block immediately evaluates `s2 != 0` (`mv t3,s2; ...; snez t0,t0`)
instead of consuming `%t13`'s published home. This loses the skip-edge
publication and reads the RHS pointer home, producing the wrong short-circuit
value before the next branch.

## Suggested Next

Implement the RV64 object-route lowering for prepared predecessor-terminator
parallel-copy publications whose carrier is `select_materialization`, starting
with immediate-to-register and same-bank register-to-register moves into the
join result home.

## Watchouts

The repair should key off prepared `join_transfer`/`parallel_copy` metadata,
move kind, source/destination homes, and value bank. Do not key off testcase
name, C source spelling, exact value ids, physical register names, block labels,
instruction indexes, objdump addresses, or log text. Keep terminator lowering
and same-width `ZExt` lowering closed.

## Proof

Audit-only packet. No code change or build proof was required, and no root
proof logs were regenerated. Evidence inspected:
`ideas/open/379_rv64_object_route_20000112_runtime_join_publication.md`,
`build/agent_state/378_step1_20000112.semantic_bir.log`,
`build/agent_state/378_step1_20000112.prepared_bir.log`,
`build/agent_state/378_step5_20000112.c4c_objdump.log`,
`build/agent_state/378_step5_20000112.qemu_strace.err`,
`build/agent_state/378_step5_20000112.qemu_strace.out`,
`build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`,
`test_before.log`, and `test_after.log`.
