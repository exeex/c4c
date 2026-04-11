# Worker Execution Rules

Use this reference for delegated implementation work.

## Required Inputs

Before editing:

1. Read the parent packet.
2. Read any worker packet file named by the parent, such as `todoA.md`.
3. Read `plan.md` only when the packet says plan context is required.

## Required Flow

1. Restate the owned slice to yourself.
2. Stay inside the file ownership named by the parent whenever possible.
3. Make the smallest coherent change that advances the delegated goal.
4. Run only the narrowest validation needed for changed files:
   - prefer `.o` target builds for production sources
   - prefer one owned test or test binary when working in tests
5. If local validation fails because the fix belongs outside your ownership,
   stop and report the blocker.

## Guardrails

1. Treat worker packet files as task descriptions, not as lifecycle state.
2. Do not silently expand into adjacent fixes.
3. Do not convert "one proof test if required" into broad testcase expansion.
4. Do not take over final integration, final regression, or lifecycle closure.
5. Do not create a commit or judge commit readiness unless the parent packet
   explicitly delegates that authority.

## Result Format

Return:

- files changed
- local validation run
- assumptions
- blockers or follow-up notes
