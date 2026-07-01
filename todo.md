Status: Active
Source Idea Path: ideas/open/443_rv64_prepared_value_operand_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Re-establish Prepared Value Evidence

# Current Packet

## Just Finished

Completed `plan.md` Step 1 evidence refresh for
`tests/c/external/gcc_torture/src/pr81503.c`.

Artifacts are under
`build/agent_state/443_step1_prepared_value_evidence/`, with the key evidence
in `pr81503.prepared-bir.txt`, `pr81503.bir.txt`, and
`pr81503.c4c-objdump.txt`.

Prepared value authority is present:

- `pr81503.prepared-bir.txt:37-38` keeps `%t18 = bir.add ...` as the stored
  value for `bir.store_global @c`.
- `pr81503.prepared-bir.txt:113` assigns `%t18` to `value_id=16` in register
  `t0`.
- `pr81503.prepared-bir.txt:190` publishes
  `store_source function=foo block=block_1 inst=8 source=%t18 status=available`
  with `source_producer=binary`, `source_producer_inst=7`, and
  `source_binary=yes`.
- `pr81503.prepared-bir.txt:205` records the select chain for `%t18` as a
  binary producer rooted at `block_1` inst 7.
- `pr81503.prepared-bir.txt:232` keeps storage for `%t18` as
  `encoding=register ... reg=t0`.
- `pr81503.prepared-bir.txt:249` records the prepared global access as
  `stored=%t18 symbol=c`.

RV64 consumer edges that must change:

- The `store_global @c` consumer must use the prepared stored value `%t18`
  from `t0`, preserving it across address materialization; current objdump
  clobbers `t0` for the global address at `pr81503.c4c-objdump.txt:78-83` and
  stores `t1` at `pr81503.c4c-objdump.txt:84`.
- Earlier binary and branch consumers show the expected operand-shuttle shape
  with `xor`/`mul`/`bne` using `t3,t4` at
  `pr81503.c4c-objdump.txt:29`, `:37`, `:44`, `:58`, `:66`, and `:116`;
  the next implementation packet should make the RV64 store-value consumer use
  the same prepared-value authority instead of stale/address registers.

## Suggested Next

Implement the RV64 lowering fix so the `store_global` value operand consumes
the prepared stored value `%t18`/`t0` before global-address materialization
clobbers the value register.

## Watchouts

- Consume prepared value authority directly; do not infer producer facts from
  testcase names, raw source shape, register accidents, or qemu exit codes.
- Do not change expectations, allowlists, unsupported markers, runtime
  comparison, or pass/fail accounting.
- Keep frame-slot call-argument publication in the separate idea `444`.
- Prepared producer authority is not missing for this case; the visible gap is
  the RV64 store-value consumer edge.

## Proof

Ran the delegated proof command exactly. It rebuilt `c4cll`, regenerated
`build/agent_state/443_step1_prepared_value_evidence/`, produced BIR,
prepared-BIR, object, objdump, linked clang/c4c binaries, ran both under qemu,
and preserved `test_after.log`.

Proof passed. `test_after.log` records `clang_rc=0` and `c4c_rc=255`; this
packet uses those only as captured evidence, not as producer authority.
