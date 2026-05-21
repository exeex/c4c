Status: Active
Source Idea Path: ideas/open/372_aarch64_pointer_valued_subobject_address_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Pointer-Local Address Publication Gap

# Current Packet

## Just Finished

Switched lifecycle from umbrella inventory idea 295 to focused idea 372 for
the current `00163` pointer-valued subobject address publication residual.

Plan-owner selected the leading Step 2 bucket because it has the smallest
concrete first bad fact: after `b = &(bolshevic.b)`, generated AArch64 still
dereferences the old pointer home for `&a`, so the final `*b` prints `42`
instead of `34`. No existing open idea exactly owns this scalar pointer-local
address publication boundary.

## Suggested Next

Execute Step 1: localize where the address value for `&(bolshevic.b)` is lost
before the final `*b` dereference.

Suggested initial proof/localization command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00163_c$' > test_after.log 2>&1
```

## Watchouts

Do not reopen or edit closed ideas 294 or 355. Do not special-case `00163`,
`bolshevic`, `b`, `a`, one stack offset, one register, or one emitted
instruction sequence. Keep `00187`, `00182`, `00205`, `00216`, `00174`,
`00218`, `00200`, and `00207` parked under idea 295 unless fresh localization
proves a handoff.

## Proof

Lifecycle-only switch. No implementation proof was run by plan-owner.
