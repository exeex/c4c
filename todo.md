Status: Active
Source Idea Path: ideas/open/373_aarch64_static_global_selected_value_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Selected Value Publication Gap

# Current Packet

## Just Finished

Switched lifecycle from umbrella inventory idea 295 to focused idea 373 for
the current selected static/global aggregate value publication residual.

Plan-owner selected the ranked Step 2 owner because it covers the largest
current surface and has `00182` as a concrete lead representative. Existing
open ideas do not exactly own the current boundary, and adjacent closed
`00182` owners cover older first bad facts rather than the current selected
value publication to scalar consumers.

## Suggested Next

Execute Step 1: localize where selected static digit values in `00182` stop
reaching the `topline`/`midline`/`botline` scalar call consumers.

Suggested initial proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00182_c$' > test_after.log 2>&1
```

## Watchouts

Do not reopen closed `00182` owners from counts alone. Do not special-case
`00182`, `print_led`, one digit array, one call target, one register, one
stack offset, or one emitted instruction sequence. Keep scalar constant-binary
stack publication (`00205` first bad fact), external call-result publication
(`00187`), scalar FP (`00174`), timeouts (`00200`, `00207`), and bit-field
local aggregate address publication (`00218`) parked under idea 295 unless
fresh localization proves a handoff.

## Proof

Lifecycle-only switch. No implementation proof was run by plan-owner.
