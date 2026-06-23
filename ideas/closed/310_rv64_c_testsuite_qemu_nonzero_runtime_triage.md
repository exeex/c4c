# RV64 C-Testsuite QEMU Nonzero Runtime Triage

## Goal

Turn the current RV64 c-testsuite runtime failures into a clear repair map for
the next backend implementation slices.

This is a triage and follow-up planning idea. It should classify the current
`QEMU_NONZERO` and timeout cases by root cause, identify the highest-value
semantic repairs, and create focused follow-up ideas under `ideas/open/`.

## Why This Exists

After the RV64 undefined-main triage and follow-up repairs, the old
`.text`-only output-contract bucket is gone. A fresh non-blocking c-testsuite
probe over the 220 allowlisted cases now shows:

```text
PASS          67
EMIT_FAIL     26
CLANG_FAIL     4
QEMU_NONZERO 122
QEMU_TIMEOUT   1
TOTAL        220
```

The important change is that `undefined reference to main` is now zero. The
remaining largest bucket is runtime behavior under qemu, not missing entrypoint
emission.

The current runtime split is:

```text
90  qemu exit 132 / illegal instruction
31  qemu exit 139 / segmentation fault
1   qemu exit 112
1   timeout
```

Representative cases from `build/rv64_c_testsuite_probe_latest/summary.tsv`:

- Illegal instruction:
  - `src/00005.c`
  - `src/00006.c`
  - `src/00007.c`
  - `src/00009.c`
  - `src/00018.c`
  - `src/00026.c`
  - `src/00027.c`
- Segfault:
  - `src/00008.c`
  - `src/00019.c`
  - `src/00032.c`
  - `src/00033.c`
  - `src/00053.c`
  - `src/00072.c`
  - `src/00073.c`
- Nonzero exit 112:
  - `src/00112.c`
- Timeout:
  - `src/00025.c`

Nearby non-runtime buckets should be kept visible but not folded blindly into
runtime work:

- 23 cases still fail closed with
  `riscv prepared module emitter does not support this prepared global storage layout`.
- 4 cases fail at assembly/link with undefined temporary labels:
  `src/00034.c`, `src/00127.c`, `src/00200.c`, and `src/00213.c`.
- 3 cases still fail before prepared handoff because semantic `lir_to_bir`
  rejected the shape.

## Inputs

Reuse the current probe artifacts if available:

- `build/rv64_c_testsuite_probe_latest/summary.tsv`
- `build/rv64_c_testsuite_probe_latest/asm/`
- `build/rv64_c_testsuite_probe_latest/bin/`
- `build/rv64_c_testsuite_probe_latest/*.qemu.out`
- `build/rv64_c_testsuite_probe_latest/*.emit.out`
- `build/rv64_c_testsuite_probe_latest/*.clang.out`

If they are stale or missing, regenerate a non-blocking sweep with the same
shape:

```sh
cmake --build --preset default
./build/c4cll --codegen asm --target riscv64-linux-gnu <case> -o <case>.s
/usr/bin/clang --target=riscv64-linux-gnu --gcc-toolchain=/usr -x assembler <case>.s -o <case>.bin -lm
/usr/bin/qemu-riscv64 -L /usr/riscv64-linux-gnu <case>.bin
```

The sweep should remain an observation artifact. Do not register all 220 cases
as required CTest coverage under this idea.

## In Scope

- Confirm the current 220-case counts from the latest RV64 c-testsuite probe.
- Classify all `QEMU_NONZERO` and timeout cases into root-cause buckets using
  emitted assembly, prepared BIR, and small representative executions.
- Distinguish at least these candidate families:
  - emitted instruction or pseudo-instruction sequence that qemu cannot run;
  - bad call ABI or return-value convention;
  - bad stack frame, local slot, spill, or reload behavior;
  - bad branch or control-flow target publication;
  - bad pointer/global address materialization;
  - libc/external call interaction, including string cases that now link;
  - genuine program nonzero exits caused by wrong semantics rather than crash.
- Pick a small set of representative cases for each meaningful family.
- Produce follow-up ideas for the next repair slices, with candidate cases and
  acceptance criteria for each slice.
- Keep the classification precise enough that the next agent can start with a
  narrow repair instead of redoing the whole sweep.

## Out Of Scope

- Implementing the runtime fixes in this triage idea.
- Treating all illegal-instruction cases as one backend bug without checking
  whether they share the same emitted assembly pattern.
- Treating all segfaults as one memory bug without separating stack, call,
  global, and pointer-address causes.
- Registering the full c-testsuite sweep as mandatory pass/fail CTest coverage.
- Weakening existing backend tests, downgrading supported-path contracts, or
  marking runtime cases unsupported merely to improve counts.
- Reopening the already-closed undefined-main, string-literal, or aggregate
  storage ideas unless their implementation is proven to be the current root
  cause of a runtime family.

## Suggested Execution Order

1. Normalize the latest probe result:
   - ignore allowlist comments;
   - record counts for pass, emit fail, clang fail, qemu nonzero, and timeout;
   - verify the undefined-main count remains zero.
2. For the `QEMU_NONZERO` cases, split first by exit status:
   - 132 illegal instruction;
   - 139 segmentation fault;
   - other nonzero exits.
3. For a small sample in each exit-status bucket, capture:
   - source summary;
   - prepared BIR function/control-flow summary;
   - assembly around calls, branches, stack frame setup, and fault-adjacent
     instructions;
   - qemu outcome.
4. Use those representatives to create semantic buckets. Prefer mechanisms over
   filename clusters.
5. Decide repair ordering by impact and confidence. Favor one narrow,
   semantic fix that unlocks many nearby cases over case-shaped patches.
6. Create at least two follow-up ideas under `ideas/open/`:
   - one for the highest-confidence illegal-instruction family;
   - one for the highest-confidence segfault or call/stack/runtime family.
7. Leave the unsupported global-storage, undefined temporary-label, and
   semantic handoff buckets as separate future work unless this triage proves
   one is tightly coupled to a runtime root cause.

## Acceptance Criteria

- The latest 220-case RV64 c-testsuite counts are recorded with the command or
  artifact path used.
- All `QEMU_NONZERO` and timeout cases are listed or summarized in
  reproducible buckets.
- Representative evidence exists for each non-trivial bucket; source-only
  regex classification is not enough when assembly or prepared BIR tells a
  clearer story.
- The triage explicitly separates:
  - illegal instructions;
  - segmentation faults;
  - intentional/non-crashing nonzero returns;
  - timeout/hang behavior.
- At least two focused follow-up ideas are created under `ideas/open/`, each
  with candidate cases, owned feature family, and acceptance criteria.
- The full c-testsuite sweep remains non-blocking evidence rather than a new
  required test suite.
- Worktree state is clean or contains only committed lifecycle/idea artifacts
  when this triage closes.

## Reviewer Reject Signals

- The result says "qemu failed" without explaining the emitted backend pattern
  behind the major buckets.
- It chases a single c-testsuite filename with a special-case fix instead of
  producing semantic repair ideas.
- Illegal instruction, segfault, timeout, and wrong nonzero return are mixed
  together as one undifferentiated bucket.
- The follow-up plan becomes a single giant "make c-testsuite pass" idea.
- Runtime progress is claimed by weakening tests, changing expected outcomes,
  or hiding supported-path failures behind unsupported diagnostics.

## Notes For The Agent

- The previous 306-309 sequence intentionally moved RV64 from entrypoint and
  data-emission blockers into real runtime behavior. Preserve that distinction.
- Store large generated case lists under `build/` or another non-root scratch
  location. Keep durable idea text concise.
- If one bucket clearly belongs to a known deferred family, write a focused
  follow-up idea instead of stretching this triage into implementation.

## Closure Summary

Closed after the active triage runbook completed all acceptance criteria. The
latest probe evidence is recorded from
`build/rv64_c_testsuite_probe_latest/summary.tsv`: 220 real source rows,
including `PASS=67`, `EMIT_FAIL=26`, `CLANG_FAIL=4`, `QEMU_NONZERO=122`, and
`QEMU_TIMEOUT=1`; undefined-main remained zero.

All 123 runtime-failure rows were classified in
`build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv` and
summarized in `triage_step4/summary.md`: external/bodyless stubs 59,
ordinary incomplete control or expression emission 23, local stack slot or
address materialization 21, global storage or address flow 8, wide/narrow
scalar storage lowering 7, and unmaterialized value or comparison result 5.
Representative backend evidence is preserved under
`build/rv64_c_testsuite_probe_latest/triage_step3/`.

Follow-up repair ideas were created under `ideas/open/`:
`311_rv64_ordinary_control_expression_completion.md`,
`312_rv64_local_stack_slot_address_materialization.md`, and
`313_rv64_external_empty_stub_policy_runtime.md`. No implementation repair was
performed under this triage-only idea.
