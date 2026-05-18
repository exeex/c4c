Status: Active
Source Idea Path: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement General Label Definition Repair

# Current Packet

## Just Finished

Step 4 from `plan.md` implemented general block-label definition emission in
`src/backend/mir/printer.hpp`.

The common MIR function printer now gathers valid `MachineBlock::successors`
target labels for a function and emits `.LBB<function>_<block>:` before each
matching block's instruction stream. This keeps single-block instruction-only
helpers stable while defining labels for actual structured branch targets. The
focused AArch64 printer test now models the same MIR contract by recording an
unconditional successor from the entry block to the target block, then asserts
that `b .LBB2_8` still prints and `.LBB2_8:` appears before the target block's
store instruction.

The representative AArch64 c-testsuite subset no longer reports undefined
temporary `.LBB...` labels. All five selected cases moved to
`[RUNTIME_UNAVAILABLE]` because no AArch64 runner is configured.

## Suggested Next

Run Step 6 broader AArch64 backend scan checkpoint, or send the route to
supervisor/plan-owner review if the representative subset plus focused backend
proof is sufficient for this slice. The next proof should continue to keep
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
  cases; do not count that as runtime pass evidence.

## Proof

Delegated proof command:

```sh
set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' && cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00005|00006|00040|00156|00220)_c$'; } 2>&1 | tee test_after.log
```

Result:

- default build succeeded
- `ctest --test-dir build -j --output-on-failure -R '^backend_'` selected 139
  tests and all 139 passed
- AArch64 scan configure/build succeeded
- representative c-testsuite subset selected 5 tests and all 5 failed as
  `[RUNTIME_UNAVAILABLE]`
- `test_after.log` contains no `undefined reference` or undefined `.LBB...`
  diagnostics for the selected subset

The nonzero proof exit is from the missing AArch64 runner, outside this
packet's owned files. The focused backend proof is green, and the old undefined
temporary-label blocker is gone for `00005.c`, `00006.c`, `00040.c`, `00156.c`,
and `00220.c`.

Proof log path: `test_after.log`.
