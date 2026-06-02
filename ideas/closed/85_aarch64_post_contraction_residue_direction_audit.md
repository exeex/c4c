# AArch64 Post-Contraction Residue Direction Audit

## Goal

Re-check AArch64 codegen layout after ideas 80-84 closed and decide the next
source-idea direction from closure notes, current residue shape, and reference
ARM layout evidence.

## Why This Exists

Ideas 80-84 completed the first post-layout contraction wave:

- dispatch publication helpers were relocated or retained intentionally
- edge-copy owner surface was contracted and retained hooks were justified
- instruction/printer table-driving candidates were exhausted
- local helper duplication tails were cleaned where a safe local owner existed
- prepared consumer wrappers were thinned without re-deriving authority

After that wave, `ideas/open/` was empty. The next step should come from the
closed ideas' own residual notes rather than reopening a completed route.

## Closure-Note Readback

| Closed idea | Signal |
| --- | --- |
| 80 | No follow-up required; remaining `dispatch_publication.*` helpers are intentionally retained shared scalar utility and current-block publication authority. |
| 81 | No new idea named; retained `dispatch_edge_copies.*` entry points are externally used edge-copy hooks. |
| 82 | Clear table-driving candidates were exhausted; remaining adjacent cleanup should start from a separate source idea. |
| 83 | Residual repeated private helper families are intentionally not promoted because no public local owner clearly owns them. |
| 84 | Retained call-boundary, store-publication, memory adapter, and scalar compare/select helpers own target-local ABI, addressing, diagnostics, scratch, or machine-record construction. |

## Current Residue Snapshot

Post-80-84 line-count concentration:

| Family | Current size | Direction |
| --- | ---: | --- |
| `calls.cpp` | 6805 | Still largest, but idea 84 says many retained helpers own target-local ABI/call-boundary authority. Needs evidence before more cleanup. |
| `memory.cpp` | 5286 | Grew after owner relocation; now has roughly 142 function surfaces. This is the clearest next audit target. |
| `alu.cpp` | 4363 | Wrapper cleanup already removed obvious redundant prepared ALU layers. Not the best immediate next route. |
| `instruction.cpp/.hpp` + `machine_printer.cpp` | about 6186 | Table-driving candidates were exhausted, but idea 82 explicitly left adjacent cleanup for a separate idea. |
| `dispatch*` | about 3300 | Significantly reduced; closure notes say retained hooks are intentional. Not the next default route. |

Reference ARM codegen remains much smaller because `emit.rs` owns broad
emission and the feature files are thin. The AArch64 backend now has a richer
prepared-consumer, machine-record, and printer surface. The remaining gap should
therefore be attacked only where the current owner boundary is demonstrably
unclear.

## Follow-Up Ideas Created

- `ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md`
- `ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md`

## Closure Note

Closed after reading ideas 80-84 closure notes, re-checking current AArch64
codegen line counts and function-surface counts, and creating two focused
follow-up audits. No implementation files changed.

The recommended next active idea is
`ideas/open/86_aarch64_memory_owner_subresponsibility_audit.md`: memory is now
the clearest post-contraction residue because prior owner relocation correctly
moved several helpers back into `memory.cpp`, but it also made memory the
second-largest owner. The audit should decide whether memory has coherent
subresponsibilities that can become implementation ideas, or whether the growth
is justified target-local residue.

