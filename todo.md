Status: Active
Source Idea Path: ideas/open/374_aarch64_local_aggregate_address_call_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Local Aggregate Address Publication Gap

# Current Packet

## Just Finished

Switched lifecycle from umbrella inventory idea 295 to focused idea 374 for
the current local aggregate address call-publication residual.

Plan-owner selected the ranked Step 2 owner because it covers the clearest
multi-representative first bad fact: `00218` passes stale `x21` instead of
`&convs` to `convert_like_real`, and `00216` crashes on an analogous stale
local aggregate address passed to `print(ls)`. Existing open ideas do not
exactly own this boundary.

## Suggested Next

Execute Step 1: localize where address-of local aggregate values stop reaching
scalar pointer call arguments.

Suggested initial proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00218_c|c_testsuite_aarch64_backend_src_00216_c)$' > test_after.log 2>&1
```

## Watchouts

Do not reopen closed direct-call or address-valued publication owners from
counts alone. Do not special-case `00218`, `00216`, `convs`, `ls`, one
function, one register, one stack offset, or one emitted instruction sequence.
Keep scalar constant/`sizeof` publication (`00205`), external call-result
publication (`00187`), scalar FP (`00174`), and timeout buckets (`00200`,
`00207`) parked under idea 295 unless fresh localization proves a handoff.

## Proof

Lifecycle-only switch. No implementation proof was run by plan-owner.
