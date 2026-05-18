Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Backend Scan Checkpoint

# Current Packet

## Just Finished

Step 6 from `plan.md` ran the broader AArch64 backend c-testsuite checkpoint
after the undefined temporary-label repair.

The delegated `-L aarch64_backend` scan configured and built successfully,
selected 220 tests, and recorded these post-repair failure-stage counts:

- `[RUNTIME_UNAVAILABLE]`: 189
- `[FRONTEND_FAIL]`: 30
- `[BACKEND_FAIL]`: 1
- remaining undefined temporary `.LBB...` diagnostics: 0

The old undefined `.LBB...` temporary-label family is absent from the broader
scan. The representative Step 1/5 cases (`00005.c`, `00006.c`, `00040.c`,
`00156.c`, and `00220.c`) remain past the old linker-symbol blocker and now
fall under `[RUNTIME_UNAVAILABLE]` when reached.

The remaining non-runtime backend failure is `00207.c`, which reports undefined
LLVM helper references: `llvm.stacksave`, `llvm.dynamic_alloca.i8`, and
`llvm.stackrestore`. Representative frontend follow-up families include scalar
machine-printer gaps such as `00027.c`/`00028.c`/`00029.c`, scalar immediate
encoding gaps such as `00024.c`/`00031.c`, and semantic `lir_to_bir` gaps such
as `00046.c`/`00140.c`/`00204.c`.

This undefined-label idea is ready for supervisor/plan-owner completion review
on compile/link capability evidence. Runtime pass evidence remains blocked by
the missing AArch64 runner, and `[RUNTIME_UNAVAILABLE]` is not counted as pass
evidence.

## Suggested Next

Ask the supervisor/plan-owner to review completion of the active undefined
temporary-label idea. The only observed remaining backend failure in the broad
scan is a separate LLVM stack helper symbol family in `00207.c`.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Do not match c-testsuite filenames, exact case names, or exact `.LBB`
  spellings instead of repairing label authority or emission semantics.
- The repair depends on structured MIR successor metadata. A branch target with
  no corresponding `MachineBlock::successors` entry will remain a separate
  label-authority bug rather than being guessed from target-owned instruction
  payloads.
- The common printer emits labels only for blocks referenced by successor
  metadata, including entry blocks if a backedge targets them. The AArch64
  assembly emitter can still emit the public function label before the function
  body.
- The broad scan now has zero `.LBB...` diagnostics; do not reopen this idea
  for the unrelated `llvm.stacksave` / dynamic alloca helper-symbol backend
  failure.
- Runtime-unavailable dominates the post-repair scan because no runner is
  configured; this is not pass evidence.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result:

- AArch64 scan configure/build succeeded
- CTest selected 220 `aarch64_backend` tests, with 0 passes and 220 failures
- post-repair counts are 189 `[RUNTIME_UNAVAILABLE]`, 30 `[FRONTEND_FAIL]`,
  and 1 `[BACKEND_FAIL]`
- remaining undefined temporary `.LBB...` diagnostics: 0
- remaining backend failure: `00207.c` with undefined LLVM stack helper
  symbols, separate from the temporary block-label family

The proof command exits nonzero due remaining failures outside this packet's
owned classification work: missing AArch64 runner, existing frontend/semantic
lowering gaps, and the separate LLVM stack helper backend-symbol family.
The old undefined temporary-label blocker is gone in the broad scan, and
`[RUNTIME_UNAVAILABLE]` is not counted as pass evidence.

Proof log path: `test_after.log`.
