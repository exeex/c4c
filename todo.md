Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Call-Boundary Preparation

# Current Packet

## Just Finished

Step 3 repaired the AArch64 floating preserved-source call-boundary
publication gap without expectation, runner, CTest contract, `plan.md`, or
source-idea edits.

The repair extends the existing call-boundary retargeting hook so an unnamed
callee-saved FPR preserved source can be matched back to the currently emitted
scalar by prepared value id. When a floating conversion result remains live in
an emitted FP/SIMD register, the later call-boundary move now publishes that
live register instead of blindly trusting a stale prepared callee-saved FPR
home. The value-id fallback is restricted to FP/SIMD preserved sources so
existing GPR preserved-home publication remains unchanged.

For the localized `00204` first bad fact, generated AArch64 for the
`fr_hfa12().a` / `fr_hfa12().b` `printf` now emits `fmov d0, d8` and
`fmov d1, d9`; the `Return values:` block advances from `0.0 12.2` to
`12.1 12.2`.

## Suggested Next

Classify the next `00204` residual in the later HFA/stdarg output corruption
that appears after the fixed `Return values:` block, then decide whether it is
still inside idea 326's call-boundary/HFA floating publication scope or needs
a separate lifecycle handoff.

## Watchouts

The delegated proof command still exits nonzero because
`c_testsuite_aarch64_backend_src_00204_c` fails later in the run, after the
targeted `Return values:` lane is fixed. Treat that as a new first bad fact,
not as persistence of the stale `d20` / live `d8` publication bug.

The value-id retarget fallback intentionally does not apply to GPR preserved
sources; the backend instruction-dispatch regression guard expects integer
successor call arguments to continue publishing from their preserved home.

## Proof

Ran the delegated Step 3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded. The selected `backend_.*` tests passed, including
`backend_aarch64_instruction_dispatch`; `c_testsuite_aarch64_backend_src_00140_c`
passed; `c_testsuite_aarch64_backend_src_00204_c` still failed later in runtime
output. `test_after.log` is the preserved proof log and records the targeted
advancement in the HFA return-value block: `12.1 12.2` now appears where the
prior first bad fact was `0.0 12.2`.
