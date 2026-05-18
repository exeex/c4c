Status: Active
Source Idea Path: ideas/open/293_aarch64_side_effect_control_value_publication_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Locate Side-Effect Publication Boundaries

# Current Packet

## Just Finished

Lifecycle switch completed: the umbrella inventory split focused source idea
`ideas/open/293_aarch64_side_effect_control_value_publication_authority.md`,
and active execution state now points to that idea.

## Suggested Next

Begin Step 1 by locating where `src/00164.c`, `src/00183.c`, and `src/00202.c`
lose side-effecting or control-selected scalar value authority before later
observable consumers.

Starter proof command:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00164|00183|00202)_c$' -j 4 --timeout 5 --output-on-failure; } > test_after.log 2>&1
```

Check for stale generated runtime processes after any timeout-protected run:

```sh
pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true
```

## Watchouts

- Do not implement under umbrella idea 284.
- Keep `src/00169.c` as boundary/support proof, not the first acceptance case.
- Do not reopen closed scalar call, call-argument, switch/control, or
  function-pointer owners without contradictory generated-code evidence.
- Keep pointer/address-heavy cases `src/00172.c` and `src/00217.c` deferred
  unless the same scalar side-effect/control publication primitive owns them.
- Do not touch expected outputs, allowlists, unsupported classifications,
  timeout policy, runner behavior, CTest registration, or build/test
  infrastructure.

## Proof

Lifecycle-only split. No implementation, build, test, or root proof logs were
run for this switch.
