Status: Active
Source Idea Path: ideas/open/220_backend_test_tree_split_bir_mir_and_prune_legacy.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Validation And Close Readiness

# Current Packet

## Just Finished

Step 6 final validation and close-readiness packet completed. The delegated
proof rebuilt the default preset, ran the independent BIR and MIR selectors,
ran the broader `^backend_` subset, and checked CMake files for stale
`backend_cli_(dump|trace)_mir` placeholders and old flat
`tests/backend/backend_(x86|aarch64)_` source paths.

Source-idea acceptance criteria status: satisfied for this runbook. The
`tests/backend/bir` and `tests/backend/mir` trees exist, own the current
BIR/prepared-BIR and AArch64 MIR tests respectively, and are selectable with
independent `bir` and `mir` CTest labels. Backend case corpus ownership is
classified in `tests/backend/OWNERSHIP.md`, disabled MIR dump/trace placeholders
are recorded as deleted stale x86/debug route debris, existing live backend
behavior is preserved by the passing backend subsets, and no new AArch64 case
coverage was enabled ahead of the markdown owner.

## Suggested Next

Supervisor should hand this active lifecycle state to the plan owner for close
decision and any required regression-guard close proof. No executor
implementation packet is currently recommended.

## Watchouts

- Remaining shared fixtures are intentionally retained under
  `tests/backend/case/` as documented input data; their location is not proof
  ownership.
- Remaining legacy corpus entries are documented as retained source inventory,
  not active BIR or MIR coverage unless a future CMake owner consumes them.
- Deprecated `--dump-mir` and `--trace-mir` x86 debug-route placeholders are
  documented as removed stale route debris; no matching CMake references remain
  in the checked backend CMake files.
- Close readiness is positive from the executor side. Final lifecycle closure
  and any broader regression-log comparison remain supervisor/plan-owner work.

## Proof

Delegated proof command:
`bash -lc 'set -o pipefail; cmake --build --preset default && ctest --test-dir build -L "bir" --output-on-failure && ctest --test-dir build -L "mir" --output-on-failure && ctest --test-dir build -R "^backend_" --output-on-failure && ! rg "backend_cli_(dump|trace)_mir" tests/backend/CMakeLists.txt tests/backend/bir/CMakeLists.txt tests/backend/mir/CMakeLists.txt && ! rg "tests/backend/backend_(x86|aarch64)_" tests/backend/CMakeLists.txt tests/backend/bir/CMakeLists.txt tests/backend/mir/CMakeLists.txt' > test_after.log 2>&1`

Result: passed. The default build completed with no work to do, `ctest -L bir`
passed 108/108 tests, `ctest -L mir` passed 27/27 tests, the `^backend_` CTest
subset passed 135/135 tests, and both negative `rg` checks found no stale MIR
dump/trace placeholders or old flat backend source-path references in the
checked CMake files. Proof log path is `test_after.log`.
