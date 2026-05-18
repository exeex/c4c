Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Against The AArch64 C-Testsuite Backend Subset

# Current Packet

## Just Finished

Step 5 from `plan.md` recorded targeted AArch64 c-testsuite subset proof for
the repaired undefined temporary-label family.

The delegated representative subset selected `00005.c`, `00006.c`, `00040.c`,
`00156.c`, and `00220.c`. All five cases now classify as
`[RUNTIME_UNAVAILABLE]` because no AArch64 runner is configured. The old
undefined temporary `.LBB...` linker diagnostics are gone from this subset, and
`test_after.log` contains no `undefined reference` or undefined `.LBB...`
diagnostics.

## Suggested Next

Run Step 6 broader AArch64 backend scan checkpoint for the active idea, keeping
`[RUNTIME_UNAVAILABLE]` separate from pass evidence.

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
- The representative subset now reaches `[RUNTIME_UNAVAILABLE]` for all five
  cases; this proves compile/link progress for the owned label family but is
  not runtime pass evidence.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'; } 2>&1 | tee test_after.log
```

Result:

- AArch64 scan configure/build succeeded
- representative c-testsuite subset selected 5 tests and all 5 failed as
  `[RUNTIME_UNAVAILABLE]`: `00005.c`, `00006.c`, `00040.c`, `00156.c`, and
  `00220.c`
- `test_after.log` contains no `undefined reference` or undefined `.LBB...`
  diagnostics for the selected subset

The nonzero proof exit is from the missing AArch64 runner, outside this
packet's owned files. The old undefined temporary-label blocker is gone for
the representative subset, and `[RUNTIME_UNAVAILABLE]` is not counted as pass
evidence.

Proof log path: `test_after.log`.
