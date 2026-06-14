# 269 Phase F4 PreparedBirModule invariants/completed_phases/notes metadata proof gate

## Goal

Create a proof gate for `PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes` before
any later private-pass-context proposal.

This is proof-gate work only. It must not demote, delete, privatize, or wrap
the metadata fields.

## Why This Exists

The post-F3 gate found these metadata fields are not ready for private
pass-context work. They need printer/status/debug preservation plus fail-closed
proof for invalid, mismatched, missing, direct-payload-read, and absent metadata
behavior before any later proposal can be safe.

## In Scope

- Map printer preservation for invariants, completed phases, and notes.
- Map status/debug preservation for those metadata fields.
- Map invalid metadata behavior.
- Map mismatched metadata behavior.
- Map missing metadata behavior.
- Map direct-payload-read behavior.
- Map absent metadata behavior.
- Name which public prepared surfaces are retained compatibility authority.

## Out Of Scope

- Metadata demotion, deletion, privatization, accessor wrapping, or adapter
  migration.
- Broad `PreparedBirModule` retirement.
- Moving target policy into BIR.
- Weakening printer, status, debug, helper/oracle, fallback, wrapper, target
  output, unsupported behavior, or baseline expectations.

## Completion Criteria

- A proof gate exists for `invariants`, `completed_phases`, and `notes` with
  printer/status/debug preservation explicitly named.
- Invalid, mismatched, missing, direct-payload-read, and absent metadata rows
  each name the expected fail-closed behavior.
- The gate records retained compatibility authority and blocks any later
  private-pass-context packet until all proof rows are satisfied.
- The result does not claim that metadata proof implies whole
  `PreparedBirModule` retirement.

## Reviewer Reject Signals

- The slice weakens printer, status, debug, helper/oracle, fallback,
  route-debug, wrapper-output, exact target-output, unsupported, or baseline
  expectations.
- The slice claims progress through expectation rewrites, helper renames,
  status/oracle relabeling, printer/debug formatting changes, or
  classification-only notes.
- The slice hides old public metadata authority behind renamed BIR fields,
  route fields, private accessors, adapters, wrappers, or compatibility helpers
  without proving the remaining public surface is only a mirror.
- The slice migrates target-owned ABI, layout, register, stack, storage,
  addressing, helper, formatting, emission, instruction spelling, wrapper, or
  exact target-output policy into BIR.
- The slice proves only one metadata field or one named fixture while invalid,
  mismatched, missing, direct-payload-read, or absent metadata rows remain
  unexamined.
- The slice authorizes broad `PreparedBirModule` retirement, broad prepared
  retirement, or metadata demotion.
