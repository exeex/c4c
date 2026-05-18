# AArch64 Backend Non-Leaf Call-Frame LR Preservation Todo

Status: Active
Source Idea Path: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reclassify Timeout Boundary

# Current Packet

## Just Finished

Completed plan.md Step 5, "Reclassify Timeout Boundary", by rebuilding the
AArch64 scan compiler and rerunning only the original 23-case timeout bucket
with an explicit 10 second CTest timeout. The original timeout bucket is now
fully reclassified:

- Pass after LR preservation: `00100.c`, `00121.c`.
- Still timeout: `00132.c`.
- Non-timeout `RUNTIME_NONZERO`: `00116.c` exits 1; `00175.c`, `00196.c`, and
  `00199.c` segfault.
- Non-timeout `RUNTIME_MISMATCH`: `00125.c`, `00131.c`, `00154.c`, `00159.c`,
  `00161.c`, `00166.c`, `00172.c`, `00178.c`, `00184.c`, `00190.c`,
  `00191.c`, `00192.c`, `00197.c`, `00201.c`, `00206.c`, and `00211.c`.

This confirms the old broad LR-preservation timeout boundary has collapsed to
two passes, one remaining timeout, and 20 quick runtime failures.

## Suggested Next

Plan-owner should decide whether to close this LR-preservation plan, deactivate
it, or split follow-on work into separate source ideas. No follow-on semantic
owner was implemented in this packet.

## Watchouts

- Likely follow-on owner groups are semantic/backend capability work, not LR
  preservation: argument/return value lowering (`00116.c`), printf/string
  literal/variadic-call behavior (`00125.c`, `00131.c`, `00166.c`, `00190.c`),
  aggregate and pointer access (`00154.c`, `00172.c`), loop/control-flow
  lowering (`00132.c`, `00161.c`, `00191.c`, `00192.c`), scalar conversions and
  sizeof/type semantics (`00175.c`, `00178.c`, `00184.c`), short-circuit calls
  (`00196.c`), static storage duration (`00197.c`), goto across block scopes
  (`00199.c`), and macro/preprocessor-adjacent cases (`00201.c`, `00206.c`,
  `00211.c`).
- `00159.c` mixes call return values and normal calls; its current mismatch is
  quick and should be grouped with follow-on call/printf semantics rather than
  the LR timeout repair.
- The stale-process check after the timeout-oriented run matched only the
  check command and its `rg` child, with no stale generated runtime, `ctest`,
  `cmake -D`, or `c4cll` process left behind.
- Do not turn the remaining failures into expectation changes, allowlist
  edits, timeout changes, CTest runner changes, or c-testsuite-specific
  shortcuts.

## Proof

Ran the delegated proof command with combined output in `test_after.log`:

```sh
cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -R 'c_testsuite_aarch64_backend_src_(00100|00116|00121|00125|00131|00132|00154|00159|00161|00166|00172|00175|00178|00184|00190|00191|00192|00196|00197|00199|00201|00206|00211)_c$' --output-on-failure --timeout 10
```

Result: build succeeded; CTest ran all 23 selected tests. `00100.c` and
`00121.c` passed. CTest exited nonzero because the reclassified boundary still
contains 21 failures: 1 timeout, 4 `RUNTIME_NONZERO`, and 16
`RUNTIME_MISMATCH`. This is sufficient for Step 5 runtime reclassification and
is not treated as an implementation blocker for this evidence-only packet.

Then ran the delegated stale-process check:

```sh
ps -eo pid,ppid,pgid,pcpu,stat,etime,cmd --sort=-pcpu | rg 'ctest|cmake -D|/workspaces/c4c/build[^ ]*(/| )[A-Za-z0-9_.+-]*\.bin|/workspaces/c4c/build[^ ]*/c4cll'
```

Result: no stale target process remained; the only matches were the stale-check
shell command itself and its `rg` child.
