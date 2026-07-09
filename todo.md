Status: Active
Source Idea Path: ideas/open/619_bir_aggregate_global_store_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Aggregate Global Handoff Evidence

# Current Packet

## Just Finished

Step 1 - Refresh Aggregate Global Handoff Evidence completed as a no-code
classification pass from current scan summaries and existing RV64 backend
case logs.

Current summary evidence:

- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv` records current
  failures for `src/pr57344-1.c`, `src/compndlit-1.c`,
  `src/ieee/20001122-1.c`, `src/pr22141-1.c`, and `src/pr39120.c`, and current
  passes for `src/991030-1.c` and the local-only guard
  `src/20020225-2.c`.
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt` still lists
  `src/pr57344-1.c`, `src/compndlit-1.c`, `src/ieee/20001122-1.c`,
  `src/pr22141-1.c`, and `src/pr39120.c`.

Current Step 1 row classification:

| Row | Current first owner | Evidence |
| --- | --- | --- |
| `src/pr22141-1.c` | BIR semantic producer, aggregate store into global/static handoff still stops before prepared object handoff; latest function `f1` fails in `store local-memory semantic family`. | `build/rv64_gcc_c_torture_backend/src_pr22141-1.c/case.log` |
| `src/compndlit-1.c` | BIR semantic producer, compound-literal aggregate assignment into global `x` still stops before prepared object handoff; latest function `main` fails in `store local-memory semantic family`. | `build/rv64_gcc_c_torture_backend/src_compndlit-1.c/case.log` |
| `src/pr57344-1.c` | BIR semantic producer, packed aggregate assignment into global array element `s[1]` still stops before prepared object handoff; latest function `main` fails in `store local-memory semantic family`. | `build/rv64_gcc_c_torture_backend/src_pr57344-1.c/case.log` |
| `src/pr39120.c` | BIR semantic producer, struct call-result assignment into global `x` still stops before prepared object handoff; latest function `main` fails in `store local-memory semantic family`. | `build/rv64_gcc_c_torture_backend/src_pr39120.c/case.log` |
| `src/ieee/20001122-1.c` | Downstream RV64/global consumer, not a BIR/prepared handoff row for Step 2 repair: current diagnostics reach prepared global data and stop on unsupported global access width. | `build/rv64_gcc_c_torture_backend/src_ieee_20001122-1.c/case.log` |
| `src/991030-1.c` | Not in current repair set: current backend object route passes, so no global-storage/global-data failure remains to split in. | `build/rv64_gcc_c_torture_backend/src_991030-1.c/case.log` |

Current in-scope repair set for Step 2 is therefore:
`src/pr22141-1.c`, `src/compndlit-1.c`, `src/pr57344-1.c`, and
`src/pr39120.c`. `src/ieee/20001122-1.c` should stay classified as a
downstream RV64/global consumer width row unless Step 2 finds missing prepared
authority despite the current `unsupported_global_data` diagnostic.

Local-only aggregate guard rows for later proof:

- `src/20020225-2.c`: current pass in
  `build/rv64_gcc_c_torture_backend/src_20020225-2.c/case.log`; earlier local
  aggregate evidence is in
  `build/agent_state/405_step1_local_aggregate_probe.log` and
  `build/agent_state/405_step1_local_aggregate.allowlist.txt`.
- `src/ieee/mul-subnormal-single-1.c`: earlier local aggregate width guard is
  in `build/agent_state/405_step2_local_aggregate_probe.log` and
  `build/agent_state/405_step2_local_aggregate.allowlist.txt`; current case
  now reaches a non-local downstream `unsupported_move_bundle_target_shape`
  stop in
  `build/rv64_gcc_c_torture_backend/src_ieee_mul-subnormal-single-1.c/case.log`.

## Suggested Next

Execute Step 2: localize the missing BIR/prepared handoff authority for the
four in-scope rows by comparing their aggregate destination/source shapes
against the local-only guard behavior. Start with the common BIR producer
diagnostic (`store local-memory semantic family` before prepared handoff) and
decide whether the missing fact is destination authority for global/static
storage, source aggregate value publication, or a classification bug in the
admission diagnostic.

## Watchouts

- Do not treat `src/ieee/20001122-1.c` as proof of a BIR/prepared repair; its
  current first owner is RV64/global consumer width after prepared global data
  is present.
- Do not pull `src/991030-1.c` into Step 2; it is currently green.
- Keep `src/20020225-2.c` and `src/ieee/mul-subnormal-single-1.c` as
  local-only aggregate guards so Step 3 does not regress idea 603 behavior
  while repairing global/static aggregate handoff.
- The four in-scope rows all report the same broad `store local-memory`
  family stop, so Step 2 needs shape inspection before implementation to avoid
  folding global/static aggregate stores into ordinary local-memory production.

## Proof

No build or test command was required by the delegated no-code evidence packet.
Proof is by current existing artifacts under
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`,
`build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, and the cited
`build/rv64_gcc_c_torture_backend/*/case.log` files. No `test_after.log` was
produced because the packet explicitly required no proof command.
