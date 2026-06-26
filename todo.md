Status: Active
Source Idea Path: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Local-Memory Metadata

# Current Packet

## Just Finished

Lifecycle activation created this executor state for `plan.md` Step 1.

## Suggested Next

Start Step 1 by locating the RV64 object-route
`unsupported_local_memory_access` diagnostic and auditing the prepared facts
available for `src/20030914-2.c`.

## Watchouts

- Do not infer local-memory bases or aggregate byte ranges from source syntax,
  testcase names, physical registers, raw object offsets, or log text.
- Keep non-register parameter homes, aggregate `va_arg`, and unrelated
  global/data work out of this runbook unless the prepared local-memory
  contract directly requires them.
- Preserve fail-closed diagnostics for adjacent unsupported shapes.

## Proof

No implementation proof yet; this is a lifecycle-only activation.
