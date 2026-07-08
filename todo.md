Status: Active
Source Idea Path: ideas/open/602_bir_local_memory_load_semantics.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Update Evidence and Handoff

# Current Packet

## Just Finished

Completed Step 5: updated the final proof summary and handoff for the current
BIR local-memory load semantics slice. No implementation files, tests,
expectations, unsupported markers, allowlists, runtime/accounting behavior,
`plan.md`, source ideas, or docs handoff files were changed.

Final proof summary:

- The current restored full RV64 gcc_torture backend-object scan is
  `473/1467` passing with `994` failures.
- The current scan has `1467` per-case logs and `994` failed-list rows under
  `build/agent_state/`.
- The local-memory load semantic family is now `73` remaining stops, down from
  the Step 2 baseline of `82` rows at `470/1467`.
- The slice moved multiple rows past the old BIR load producer stop without
  weakening adjacent guard ownership.

Delegated load-family proof rows:

- `src/20000706-4.c` old first owner was `bar` failing in `load local-memory
  semantic family`; current full-scan owner is `[RV64_BACKEND_RUNTIME_MISMATCH]`
  with `c4c_exit=Segmentation fault`. This row cleared the old BIR load-family
  producer stop and exposed downstream runtime ownership.
- `src/20010129-1.c` old first owner was `foo` failing in `load local-memory
  semantic family`; current full-scan owner is prepared/RV64
  `unsupported_call_abi`. This row cleared the old BIR load-family producer
  stop and exposed downstream ABI/RV64 ownership.
- `src/20041124-1.c`, `src/20011008-3.c`, and `src/920625-1.c` still fail in
  `load local-memory semantic family`. They remain aggregate/member,
  va_arg/byval, or complex pointer-load breadth work for later BIR load
  producer slices.

Full-scan breadth evidence:

- The Step 2 failure map classified `82` rows in the local-memory load semantic
  family at the `470/1467` baseline.
- The restored full scan now has `73` `load local-memory semantic family`
  per-case logs, so the load-family bucket shrank by `9` rows while total pass
  count rose from `470` to `473`.
- Current artifacts therefore show additional same-family movement beyond the
  two named delegated load rows, but the overwritten mutable per-case logs do
  not preserve enough pre-repair row identity to claim exact names for every
  moved row defensibly from this packet alone.

Guard rows retained non-load ownership:

- `src/20010605-2.c` stayed in `store local-memory semantic family`.
- `src/20030717-1.c` stayed in `gep local-memory semantic family`.
- `src/20180921-1.c` stayed in `alloca local-memory semantic family`.
- `src/20000217-1.c` stayed at prepared move-bundle classification.
- `src/20021204-1.c` stayed at RV64 `unsupported_local_memory_access`.
- `src/20030910-1.c` stayed at `unsupported_terminator_fragment`.
- `src/20000706-1.c` stayed at `unsupported_move_bundle_target_shape` /
  stack-offset publication rejection.

Downstream handoff owners exposed by the cleared rows:

- `src/20000706-4.c` now reaches `[RV64_BACKEND_RUNTIME_MISMATCH]` with
  `c4c_exit=Segmentation fault`; this is runtime mismatch ownership, not BIR
  load producer ownership.
- `src/20010129-1.c` now reaches prepared/RV64 `unsupported_call_abi`; this is
  ABI/RV64 ownership, not BIR load producer ownership.

## Suggested Next

Ask the supervisor and plan-owner to decide closure readiness for this active
plan. The local BIR load semantics slice has final evidence and handoff notes,
but closure remains a lifecycle decision because `73` load-family stops and
downstream runtime/ABI owners remain.

## Watchouts

- Keep named cases as probes only, not match keys.
- `src/20000706-4.c` now reaches runtime mismatch. Do not treat that as a
  runtime/accounting fix in this lifecycle; it is only proof that the old BIR
  load producer stop cleared.
- `src/20010129-1.c` now reaches prepared/RV64 `unsupported_call_abi`; that is
  downstream of semantic BIR production and outside this packet.
- The remaining selected load rows still need later load producer work for
  aggregate/member, va_arg/byval, or complex pointer shapes.
- Remaining limitations include the `73` current load-family stops and the
  inability to reconstruct exact identities for every unnamed row in the
  `82 -> 73` movement from overwritten mutable pre-repair logs.
- This packet does not claim ownership of runtime mismatch, ABI/RV64, store,
  GEP, alloca, prepared authority, timeout, accounting, or expectation policy.
- Do not weaken store/GEP/alloca/prepared/RV64 guard ownership to make the
  12-row total pass count improve.
- Do not overclaim the unnamed `82 -> 73` bucket movement as exact per-row
  proof; the current full scan proves aggregate breadth, while the named
  delegated rows prove concrete downstream handoff identities.

## Proof

Step 5 reused the Step 4 evidence proof over existing restored full-scan
artifacts:

```sh
find build/rv64_gcc_c_torture_backend -maxdepth 2 -name case.log | wc -l
awk -F '\t' 'NR>1 {c[$1]++} END {for (k in c) print k,c[k]}' \
  build/agent_state/rv64_gcc_c_torture_backend_summary.tsv | sort
wc -l build/agent_state/rv64_gcc_c_torture_backend_failed.txt
rg -l 'load local-memory semantic family' \
  build/rv64_gcc_c_torture_backend/*/case.log | wc -l
```

Final observed state:

- `1467` per-case logs.
- Summary counts: `pass 473`, `fail 994`.
- Failed-list rows: `994`.
- Current `load local-memory semantic family` logs: `73`.

Supervisor-selected Step 5 proof command run after this `todo.md` update and saved in
`test_after.log`:

```sh
rg 'Step 5|final proof summary|473/1467|994|73|20000706-4|20010129-1|remaining limitations|Suggested Next|close' todo.md
```
