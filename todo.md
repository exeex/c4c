Status: Active
Source Idea Path: ideas/open/619_bir_aggregate_global_store_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Localize The Missing Handoff Authority

# Current Packet

## Just Finished

Step 2 - Localize The Missing Handoff Authority completed as a no-code
evidence pass using current case logs plus focused semantic/prepared BIR dumps
under `build/agent_state/619_step2_localize_handoff_authority/`.

All four in-scope rows are blocked before prepared handoff publication: both
`--dump-bir` and `--dump-prepared-bir` return rc `1` with the same semantic
`lir_to_bir` failure already seen in the backend case logs. There is therefore
no prepared global/static destination authority to inspect yet, and no
downstream RV64/global-data classifier has first ownership for these rows.

Rows grouped by concrete first missing authority:

| Missing authority group | Rows | Concrete missing fact | Evidence |
| --- | --- | --- | --- |
| BIR semantic producer: aggregate assignment into direct global/static object from aggregate literal or local aggregate value | `src/pr22141-1.c`, `src/compndlit-1.c`, `src/pr57344-1.c` | Semantic BIR does not produce a supported aggregate-width `store_global` or equivalent elementwise global/static copy for a whole-aggregate assignment destination (`u`, `x`, or `s[1]`). The first absent fact is destination authority for aggregate global/static storage, not prepared publication. Source variants are compound literal (`pr22141-1.c`, `compndlit-1.c`) and local aggregate object (`pr57344-1.c`). | `build/agent_state/619_step2_localize_handoff_authority/pr22141-1.dump-bir.stderr.txt`, `build/agent_state/619_step2_localize_handoff_authority/compndlit-1.dump-bir.stderr.txt`, `build/agent_state/619_step2_localize_handoff_authority/pr57344-1.dump-bir.stderr.txt`; current case logs under `build/rv64_gcc_c_torture_backend/` |
| BIR semantic producer: aggregate assignment into direct global/static object from call result | `src/pr39120.c` | Semantic BIR does not publish a supported aggregate call-result value as the source for whole-aggregate global/static assignment into `x`. This is still a BIR semantic production gap because prepared BIR is unreachable; the source-value authority is call-result aggregate preservation plus destination global/static aggregate store authority. | `build/agent_state/619_step2_localize_handoff_authority/pr39120.dump-bir.stderr.txt`; `build/rv64_gcc_c_torture_backend/src_pr39120.c/case.log` |

Local-only aggregate guard comparison:

- `src/20020225-2.c` now passes the backend object route in
  `build/rv64_gcc_c_torture_backend/src_20020225-2.c/case.log`; focused dumps
  also succeed with rc `0` for both semantic and prepared BIR. Its BIR contains
  local union stores/loads (`bir.store_local %lv.a.0, double ...`,
  `bir.store_local %lv.a.0, i32 ...`, `bir.load_local ...`), and prepared
  metadata publishes local frame-slot addressing under
  `--- prepared-addressing ---`.
- `src/ieee/mul-subnormal-single-1.c` also reaches semantic and prepared BIR
  with rc `0`; current object failure is downstream
  `unsupported_move_bundle_target_shape` in
  `build/rv64_gcc_c_torture_backend/src_ieee_mul-subnormal-single-1.c/case.log`.
  Its successful BIR/prepared dumps include local union stores plus scalar
  `bir.load_global`/`bir.store_global @ok` facts, so it is not evidence that
  whole-aggregate global/static assignment is already produced.
- Prior guard evidence remains in
  `build/agent_state/405_step1_local_aggregate_probe.log`,
  `build/agent_state/405_step1_local_aggregate.allowlist.txt`,
  `build/agent_state/405_step2_local_aggregate_probe.log`, and
  `build/agent_state/405_step2_local_aggregate.allowlist.txt`. Those earlier
  stops were local-memory access classification, not the current four-row
  semantic BIR aggregate-global assignment gap.

Step 3 should target BIR semantic production first: add or adjust the semantic
lowering for whole-aggregate assignments into direct global/static storage,
preserving source-value authority for compound literals, local aggregate
objects, and aggregate call results. Prepared handoff publication is the next
inspection point only after these rows can produce semantic BIR; downstream
classification is not the first owner for the current in-scope repair set.

## Suggested Next

Execute Step 3 as a bounded code packet in the BIR semantic producer. Start
with direct whole-aggregate assignment to global/static destinations and prove
at least two representatives progress beyond the prior `--dump-bir` failure
without changing local-only aggregate behavior.

## Watchouts

- Do not treat `src/ieee/20001122-1.c` as proof of a BIR/prepared repair; its
  current first owner is RV64/global consumer width after prepared global data
  is present.
- Do not pull `src/991030-1.c` into Step 2; it is currently green.
- Keep `src/20020225-2.c` and `src/ieee/mul-subnormal-single-1.c` as
  local-only aggregate guards so Step 3 does not regress idea 603 behavior
  while repairing global/static aggregate handoff.
- The shared diagnostic still says `store local-memory semantic family`, but
  the first missing authority for these rows is global/static aggregate
  destination production in semantic BIR. Do not repair by widening ordinary
  local-memory classification or by adding named-case exceptions.
- `src/pr39120.c` adds source-value risk: the aggregate source is a call
  result, so Step 3 should avoid a repair that only handles literal or local
  aggregate sources.

## Proof

No build or regression test command was required by the delegated no-code
localization packet. Diagnostic commands run:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu <case>`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu <case>`

Artifacts were written under
`build/agent_state/619_step2_localize_handoff_authority/` for
`pr22141-1`, `compndlit-1`, `pr57344-1`, `pr39120`, `20020225-2`, and
`ieee_mul-subnormal-single-1`. The four in-scope rows have rc `1` for both
dump modes; the two guard rows have rc `0` for both dump modes. No
`test_after.log` was produced because no regression proof command was
delegated.
