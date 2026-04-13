# X86 Backend Failure Recovery

Status: Closed
Closed: 2026-04-13
Disposition: Closed by route reset and superseded by `ideas/open/46_backend_reboot_bir_spine.md`

## Why This Was Closed

This idea started as an x86-specific recovery effort, but the current repo
state no longer matches that scope.

During execution we removed the backend-wide `try_emit_*`, direct-dispatch,
and legacy-lowering escape hatches across x86, AArch64, and RISC-V. Once that
route was deleted, the remaining work stopped being "repair x86 failures" and
became a broader backend reboot problem:

- rebuild a truthful `lir_to_bir` semantic lane
- separate semantic lowering from target-dependent legalization
- make target backends ingest prepared BIR instead of raw/direct LIR fallbacks

That is a different initiative from the original x86 recovery packet, so this
idea is intentionally concluded and archived rather than stretched to cover the
new backend-wide reboot.

## What Landed Before Closure

- removed backend `try_emit_*` / direct-dispatch escape hatches
- removed the old `legacy-lowering` route
- reduced backend build wiring to a bootstrap path centered on `lir_to_bir`
- established an initial BIR bootstrap with `i1` in semantic BIR
- added target-dependent `prepare/legalize` policy so x86/i686/aarch64/riscv64
  currently promote `i1 -> i32`
- reset `bir_validate` to structural checks only, leaving type legality to
  `prepare` and target backend ingestion

## Work Handed Off

The follow-on work now lives in `ideas/open/46_backend_reboot_bir_spine.md`.
That idea owns:

- semantic BIR coverage for params, compare, branch, select, and phi
- memory, globals, and call lowering
- `prepare/legalize`, stack layout, liveness, and regalloc
- prepared-BIR ingestion contracts for x86, aarch64, and riscv64

This archived idea should not be reopened for new backend-wide reboot work.
