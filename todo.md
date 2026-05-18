# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch To Focused Idea

# Current Packet

Step 4 completed the timeout triage. The next packet should be plan-owner
lifecycle work for creating the focused ideas and switching out of this
umbrella inventory plan. Preserve the no-implementation umbrella rule.

## Just Finished

- Completed plan.md Step 4, "Defer Or Split Timeout Work", using existing
  artifacts only.
- The classified scan has 23 `TIMEOUT` cases:
  `00100.c`, `00116.c`, `00121.c`, `00125.c`, `00131.c`, `00132.c`,
  `00154.c`, `00159.c`, `00161.c`, `00166.c`, `00172.c`, `00175.c`,
  `00178.c`, `00184.c`, `00190.c`, `00191.c`, `00192.c`, `00196.c`,
  `00197.c`, `00199.c`, `00201.c`, `00206.c`, and `00211.c`.
- Generated `.s` files are present for all 23 timeout cases; no matching
  `.run-output` files were present.
- All 23 generated `.s` files contain at least one `bl` call, and none of the
  inspected generated functions save/restore `x30`/link register before `ret`.
  This is enough to treat the timeout family as dominated by AArch64 non-leaf
  call-frame/link-register preservation rather than ordinary per-test runtime
  mismatches.
- Source-shape groups:
  - Simple user-function calls without stdio: `00100.c`, `00116.c`, `00121.c`.
    These are the cleanest evidence because the timeout does not depend on
    `printf`; `main` calls another function and then returns through a clobbered
    link register.
  - Direct `printf`-only or mostly linear stdio cases: `00125.c`, `00131.c`,
    `00166.c`, `00178.c`, `00184.c`, `00201.c`, `00206.c`, `00211.c`.
    These also expose known string-literal/variadic-call weaknesses, but the
    timeout classification itself is plausibly explained by non-leaf return.
  - Loop/control-flow plus call cases: `00132.c`, `00161.c`, `00191.c`,
    `00192.c`, `00196.c`, `00199.c`. Some show additional loop predicate or
    short-circuit/goto risks, so they should not be first proof targets for the
    timeout owner.
  - Aggregate/pointer/static/global/value-shape plus call cases: `00154.c`,
    `00172.c`, `00175.c`, `00197.c`. These are compound and should remain
    downstream of the simple call-frame proof.
  - Multi-function call chains: `00159.c`, `00190.c`, `00196.c`, `00197.c`,
    `00199.c`; useful later for caller/callee interaction, but too compound for
    an initial narrow repair.
- Decision: split a safe focused idea for `aarch64_backend_nonleaf_call_frame_lr_preservation`
  using the simple call cases (`00100.c`, `00116.c`, `00121.c`) as initial
  evidence/proof candidates. Quarantine the rest of the timeout bucket from the
  ordinary runtime-mismatch stream until that owner is fixed and the scan is
  reclassified. Do not implement timeout fixes in this umbrella plan.

## Suggested Next

- Plan-owner lifecycle packet: create focused idea(s) from the completed
  inventory, especially `aarch64_backend_nonleaf_call_frame_lr_preservation`,
  then switch out of this umbrella plan to focused repair work.

## Watchouts

- Do not hand broad runtime scans to executors without timeout and stale-process
  cleanup.
- Do not count timeout/hang cases as ordinary repair packets.
- Do not change expected outputs, allowlists, unsupported classifications, or
  CTest behavior to improve counts.
- Keep printer coverage, semantic `lir_to_bir` admission, and runtime behavior
  ideas separate; the scan's `FRONTEND_FAIL` label does not mean C frontend
  ownership here.
- Avoid choosing compound integration cases (`00168.c`, `00213.c`, `00216.c`)
  as initial focused repair targets.
- The `RUNTIME_MISMATCH` samples with empty actual output are still not proof
  that `printf` alone is broken; loop predicates often prevent reaching the
  call, and the generated call sequence also lacks credible format-string
  addressing.
- The timeout set still contains secondary defects: string-literal/variadic
  call lowering, loop predicate/short-circuit control flow, local stack slots,
  aggregates/pointers, static globals, and goto behavior. Those should remain
  quarantined until the non-leaf link-register owner no longer masks runtime
  behavior.
- `00100.c`, `00116.c`, and `00121.c` are the best narrow timeout probes
  because they avoid stdio and isolate calls/returns.

## Proof

Used existing artifacts only:

```bash
/tmp/c4c_aarch64_backend_scan_212.classified
/tmp/c4c_aarch64_backend_scan_212.log
tests/c/external/c-testsuite/src/{00100,00116,00121,00125,00131,00132,00154,00159,00161,00166,00172,00175,00178,00184,00190,00191,00192,00196,00197,00199,00201,00206,00211}.c
build-aarch64-scan/c_testsuite_aarch64_backend/src/{00100,00116,00121,00125,00131,00132,00154,00159,00161,00166,00172,00175,00178,00184,00190,00191,00192,00196,00197,00199,00201,00206,00211}.c.s
```

Re-derived the 23 `TIMEOUT` cases from the classified scan/log, confirmed all
23 generated assembly files contain `bl`, and found no `x30`/link-register
save/restore in the timeout sample assembly. No broad CTest was run. No
generated binaries were run manually. No `test_after.log` was produced because
this delegated packet was artifact-inspection only and the proof contract
explicitly requested existing artifacts.
