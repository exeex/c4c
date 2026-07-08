Status: Active
Source Idea Path: ideas/open/608_prepared_global_data_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh prepared/global authority inventory

# Current Packet

## Just Finished

- Activated Step 1 (`Refresh prepared/global authority inventory`) from
  `plan.md`.

## Suggested Next

Delegate Step 1 to an executor to refresh the prepared/global authority
allowlist, classify current residual ownership, and choose the first
implementation family for Step 2.

## Watchouts

- Keep this route in prepared/global authority. Do not fold RV64 global symbol
  emission, relocation-record emission, or access-width lowering into this
  plan.
- Prior lifecycle notes say mixed object data was split out from `608`; Step 1
  should verify current residual ownership before editing code.
- Do not change expectations, unsupported markers, allowlists, timeout or
  accounting files as progress.

## Proof

- Lifecycle activation only; no build or test proof was run.
