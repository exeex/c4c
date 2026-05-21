Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Handoff Lifecycle

# Current Packet

## Just Finished

Step 3 selected the next focused owner for implementation outside this
umbrella inventory. No existing open idea exactly owns the current `00216`
first bad fact: the closed idea 374 local aggregate address-call owner repaired
the earlier stale `print(ls)`/`&local_aggregate` call publication shape, while
the current crash is later inside `foo` through an apparently uninitialized
stack-slot-derived pointer used by a local aggregate copy/load-local-memory
path.

Created focused source idea:
`ideas/open/379_aarch64_local_aggregate_copy_load_publication.md`.

Owner decision: idea 379 owns the current non-timeout residual because it is
centered on `00216` `foo` local aggregate initializer/copy/pointer-load and
flexible-array wrapper access lowering, with acceptance requiring the current
`ldrb w9, [x10]` segfault to advance without filename-specific matching and
without expectation, runner, timeout, allowlist, CTest, or proof-log changes.

The Step 2 classification details remain the evidence for this selection.
`00216` is a runtime nonzero/crash bucket, not a compile, assembler, linker,
runner, timeout, or expectation bucket. The delegated focused proof reproduces
`[RUNTIME_NONZERO] .../00216.c exit=Segmentation fault`.

First bad fact: the crash is inside `foo`, before `test_compound_with_relocs`
or `test_multi_relocs` can execute. A batch debugger run on
`build/c_testsuite_aarch64_backend/src/00216.c.bin` stops at
`foo+1464`, instruction `ldrb w9, [x10]`. In the generated artifact
`build/c_testsuite_aarch64_backend/src/00216.c.s`, the corresponding sequence
loads a pointer from a stack slot that has no prior local initialization in the
function prologue path:

```asm
ldr x13, [sp, #496]
add x13, x13, #0
str x13, [sp, #256]
ldr x9, [sp, #256]
str x9, [sp, #992]
ldr x10, [sp, #992]
ldrb w9, [x10]
```

Relevant source shape is `tests/c/external/c-testsuite/src/00216.c` `foo`,
which combines local aggregate initializers, local struct copies/pointer
loads, flexible-array wrapper access, and nested aggregate copies:
`struct U lu1 = {3, ls, ...}`, `struct U lu2 = {3, (ls), ...}`,
`const struct S *pls = &ls`, `struct S ls21 = *pls`,
`struct U lu22 = {3, *pls, ...}`, `struct V lv2 = {(struct S)w->t.s, ...}`,
and `struct V lv3 = {..., ((const struct W *)w)->t.t, ...}`. The current crash
evidence points to local aggregate copy/load-local-memory address publication
inside `foo`; the later global relocation and function-pointer-table portions
of `00216` remain adjacency checks, not the first crash.

Secondary artifact note: `test_multi_relocs` currently emits a suspicious
table dispatch sequence that loads all three table entries but always calls
the second entry with `blr x21`; however, the debugger proves the runtime never
reaches that function before the current `foo` segfault.

`00200` and `00207` are quarantined as timeout-only residuals from Step 1.
They should stay out of the next non-timeout owner unless the supervisor
selects a timeout-specific route. No expectation, timeout-policy, runner,
allowlist, CTest registration, or implementation changes were made.

## Suggested Next

Step 4 should switch lifecycle state from umbrella idea 295 to focused idea
379 before any implementation work begins. Preserve the parked buckets for
later inventory reactivation:

1. `00216` follow-on relocation/function-pointer-table audit only after the
   `foo` crash advances; generated `test_multi_relocs` already suggests an
   index-insensitive indirect-call table dispatch, but it is not yet the first
   failing fact.
2. Timeout-specific owner for `00207` dynamic stack/VLA fixed-slot behavior or
   `00200` shift/type-promotion timeout only if the supervisor intentionally
   parks the non-timeout crash and selects a timeout route.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. `00174` remains absent from the Step 1 backend
failure list and should stay closed unless fresh proof contradicts that
boundary. `00216` should not be solved by a named-case shortcut; idea 379
requires a semantic/backend lowering rule for valid local aggregate
copy/load-local-memory address or value publication. Do not fold later
compound relocation/function-pointer-table behavior into idea 379 until the
current `foo` crash advances and fresh first-bad-fact evidence justifies that
handoff.

## Proof

Ran the delegated Step 2 focused proof command and preserved output in
`test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_00216_c$'; } > test_after.log 2>&1
```

The command exited 8 because the focused test reproduced the expected
`RUNTIME_NONZERO` segmentation fault. This is sufficient for classification:
the slice is intentionally not acceptance-ready for implementation and leaves
the failing proof in `test_after.log`.
