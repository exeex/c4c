Status: Active
Source Idea Path: ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Edge-Copy And Block-Entry Bookkeeping

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and `todo.md` for
`ideas/open/edge-copy-block-entry-bookkeeping-prealloc.md`.

## Suggested Next

Start Step 1 by inventorying AArch64 edge-copy and block-entry bookkeeping,
then choose one narrow helper boundary and its destination layer: prealloc for
Prepared fact consumption or shared MIR for target-independent machine-record
bookkeeping.

## Watchouts

Keep concrete target move emission, register constraints, instruction records,
target operands, and move diagnostics target-local. Do not combine this with
call-boundary classification, formal-entry publication, or operand decoding
migrations.

## Proof

Lifecycle-only activation. No build, ctest, or implementation proof was run.
