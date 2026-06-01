Status: Active
Source Idea Path: ideas/open/68_aarch64_large_owner_residue_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory High-Residue Owners

# Current Packet

## Just Finished

Step 1 - Inventory High-Residue Owners completed the current line-count pass
for `src/backend/mir/aarch64/codegen/` and recorded one-line responsibilities
for the high-residue owners named by `plan.md`.

Current owner inventory:

| Owner | Lines | One-line responsibility |
| --- | ---: | --- |
| `calls.cpp` | 6668 | Lowers prepared call plans, ABI boundary moves, argument/result preservation, indirect/direct calls, and call-boundary scalar-state retargeting/publication. |
| `memory.cpp` | 4526 | Builds and lowers prepared load/store memory records for locals, globals, stack/frame addressing, pointer-value bases, wide transport memory traffic, and stack-value publications. |
| `alu.cpp` | 4246 | Prepares and lowers scalar integer/floating ALU and unary operations, scalar operand/result records, emitted scalar register state, and scalar-control value materialization. |
| `machine_printer.cpp` | 2309 | Prints selected AArch64 machine instruction records into assembly text, including memory/address spelling, branches, atomics, frame code, scalar ops, calls, and call-boundary moves. |
| `instruction.hpp` | 1950 | Defines the AArch64 machine-record schema: operand kinds, opcode/status/error enums, instruction-family records, prepared-result wrappers, effects, and instruction variants. |
| `instruction.cpp` | 1828 | Implements record naming, opcode-to-printer mapping, selection-status helpers, aggregate copy/lane helpers, branch/scalar/frame/address record constructors, and unsupported-record construction. |

The source idea's high-residue list is still complete for the largest named
owners: these six files remain the largest broad AArch64 codegen owner surfaces.
The full directory line-count pass also found comparable additional owners:
`i128_ops.cpp` (2540 lines) owns i128 transport/pair/shift/compare/runtime-helper
record preparation, lowering, and printing; `comparison.cpp` (2079 lines) owns
branch and compare-condition preparation/lowering plus dispatch-branch fusion
hooks; `f128.cpp` (2072 lines) owns f128 carrier/runtime-helper records,
lowering, and printing. Near-comparable but below the current named floor are
`cast_ops.cpp` (1678 lines), `dispatch_publication.cpp` (1523 lines), and
`variadic.cpp` (1505 lines).

## Suggested Next

Proceed to Step 2 by comparing the audited AArch64 owners against the reference
ARM codegen layout and existing shared backend/prealloc prepared-fact surfaces.

## Watchouts

- Audit-only route: do not edit implementation files for this packet.
- Keep routine findings in `todo.md` until they are ready for durable
  classification or follow-up ideas.
- Do not treat line-count reduction as progress for this idea.
- Additional comparable owners are noted for supervisor routing, but this packet
  did not expand the audited-owner set beyond the Step 1 inventory scope.

## Proof

No build/test required for this audit-only `todo.md` update.
Proof command: `git diff --check -- todo.md`
Result: passed.
