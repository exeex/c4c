# Legacy Capability Coverage Ledger

Status: scaffold. This ledger prevents design completion from being claimed
while a legacy capability has no new owner. `Mapped` means a proposed document
exists; it does not mean the behavior has been exhaustively reviewed.

| Legacy capability family | Proposed owner | State |
|---|---|---|
| BIR identities, blocks, instructions, values, terminators | `core/` and verifier | Existing scaffold/code; needs contract review |
| Route 1-8 CFG/publication/comparison/call observations | `analysis/*` plus canonical passes | Mapped |
| Legal forms and scalar normalization | `passes/legalize`, `passes/scalar` | Mapped |
| CFG and control-flow transformation | `analysis/cfg`, `passes/cfg` | Mapped |
| Phi/SSA semantics | `passes/ssa`, `mir/passes/out_of_ssa` | Mapped |
| Loads, stores, addressing, atomics, pointer freshness | `passes/memory`, analyses, address plan | Mapped |
| Aggregate values/copies/GEP | `passes/aggregate` | Mapped |
| Intrinsics | `passes/intrinsics`, runtime-helper plan | Mapped |
| Comparisons and selects | `passes/scalar`, dominance/provenance analyses | Mapped |
| ABI parameter/result classification | `preparation/abi` | Mapped |
| Calls, call moves, returns, publications | call plan and MIR call lowering | Mapped |
| Variadics | variadic plan, call lowering, prologue/epilogue | Mapped |
| Inline assembly | canonical opaque op, inline-asm plan, instruction selection | Mapped |
| i128/f128 and runtime helpers | scalar/intrinsic passes and helper plan | Mapped |
| Liveness and allocation intervals | revision-bound BIR liveness/interference analysis at `S23`; BIR regalloc consumes it | Mapped; exact revision binding required |
| Register allocation and value homes | shared BIR pseudo-physical allocator at `S24`; MIR only maps verified abstract homes | Mapped |
| Spill/reload and stack slots | BIR spill/reload insertion at `S25` owns abstract spill identities and retry invalidation; later frame/MIR maps verified abstractions | Mapped |
| Frame, local storage, dynamic stack | MIR frame layout and prologue/epilogue | Mapped |
| Object data and relocations | `mir/emission` | Mapped |
| Prepared printer and debug rendering | `diagnostics` | Mapped |
| Prepared lookups/agreement/contract verifier | typed plans and stage verifiers | Mapped; exact keys pending |
| Target emitters: AArch64, RISC-V, x86 | `mir/targets/*`, instruction selection through emission | Mapped; exhaustive matrices pending |

## Mandatory exhaustive review work

1. Inventory every file and externally used symbol under `src/backend/legacy/`.
2. Assign each behavior to exactly one semantic owner and any number of
   read-only consumers.
3. Record exact input/output fields, error behavior, target differences, and
   adjacent same-feature tests.
4. Mark duplicated legacy authority for deletion rather than reproducing it.
5. Expand the object-emission, printer, and per-target scaffolds into exhaustive
   contracts before the architecture can become `accepted`.
