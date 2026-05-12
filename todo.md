Status: Active
Source Idea Path: ideas/open/202_hir_generated_member_payload_structured_miss.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Trace Generated-Member Authority

# Current Packet

## Just Finished

Lifecycle switch only: parked idea 195 as open/blocked and activated idea 202
as the current blocker with a fresh runbook. No implementation has started for
Step 1.

## Suggested Next

Begin plan Step 1 by tracing the generated-member route in
`src/frontend/hir/impl/expr/scalar_control.cpp`, especially the transition from
structured owner/member lookup to rendered
`find_struct_static_member_decl` / `find_struct_static_member_const_value`
fallback.

## Watchouts

- Idea 195 remains open but inactive/blocked until ideas 201 and 202 are
  resolved or explicitly fenced.
- Do not close idea 195, run backend restart work, or advance milestone
  validation from this blocker runbook.
- Treat a rendered owner/member fallback on metadata-rich generated-member
  paths as a blocker unless it is fenced as explicit no-metadata compatibility.
- Do not claim progress through expectation changes, unsupported markings, or
  helper renames that preserve rendered owner/member authority.

## Proof

Lifecycle-only switch. No build or test was run because no implementation files
changed.
