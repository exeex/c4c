Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish direct global-symbol base-plus-offset authority

# Current Packet

## Just Finished

- Completed Step 2 (`Publish supported prepared global memory facts`) from
  `plan.md`.
- Fixed prepared global memory fact publication for offset-bearing
  `LoadGlobalInst`/`StoreGlobalInst` inputs by avoiding double application of
  instruction byte offsets while preserving explicit GEP base-plus-offset
  facts.
- Repaired prepared memory access position lookups for final prepared BIR when
  out-of-SSA materialization shifts global load/store instruction indices.
  The repair is conservative and only remaps uniquely matched global-symbol
  memory facts.
- Focused movement: `src/strlen-7.c`, `src/20000703-1.c`, and `src/pr58662.c`
  now compile through the prior prepared/global memory authority stop;
  `src/20041218-1.c` now reaches a downstream same-module call ABI owner.
  Additional Step 2 rows `src/20140212-1.c`, `src/20190228-1.c`,
  `src/pr57861.c`, and `src/pr58431.c` also moved through the prior stop.

## Suggested Next

Delegate Step 3 (`Publish direct global-symbol base-plus-offset authority`) to
publish direct global-symbol base-plus-offset memory addressing facts from
proven producer inputs. Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

## Watchouts

- `src/pr36034-1.c` and `src/pr91137.c` still report the prepared global
  memory fact diagnostic after this slice; they appear to need a separate
  follow-up within the same family rather than testcase-shaped handling.
- `src/20060930-2.c` and `src/pr64756.c` now reach prepared move-bundle
  consumer authority, and `src/20041218-1.c` reaches same-module call ABI; keep
  those downstream owners out of Step 3 unless the supervisor changes route.
- Keep unsupported-width policy, selected object-data, RV64 global symbol
  emission, relocation-record emission, and expectation/allowlist changes out
  of the next packet.

## Proof

- Ran the delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'
```

- Result: passed, 346 backend tests, 0 failures. The supervisor accepted this
  Step 2 proof and rolled the canonical backend baseline forward to
  `test_before.log`; no separate after-log artifact is required for the next
  packet after that handoff.
- Focused representative sanity checks:
  `src/strlen-7.c`, `src/20000703-1.c`, and `src/pr58662.c` compile to RV64
  objects; `src/20041218-1.c` progresses to
  `unsupported_call_abi: RV64 object route requires supported ordinary
  same-module call ABI/result lowering`.
