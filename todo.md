Status: Active
Source Idea Path: ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The Unresolved Type Boundary

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/282_cpp_dependent_template_cast_return_ir_type_repair.md`.

## Suggested Next

Execute Step 1: reproduce the target failure, inspect HIR/debug dumps for
`convert<long, short>`, and identify where the materialized specialization
loses the function return type or dependent `static_cast<T>` target type.

## Watchouts

- Fix the general dependent-template cast/return path, not a named testcase.
- Do not weaken the positive-case runtime harness, expected output, or test
  mode.
- Do not hide unresolved `?` type state in the IR printer.
- Do not fold in reference alias C-style cast repair, C aggregate function
  pointer ABI work, or AArch64 fp128/vararg crash triage.

## Proof

Lifecycle-only activation; no build or tests run.
