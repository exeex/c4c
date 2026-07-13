# Prologue and Epilogue Realization

Status: scaffold. Consumes final allocation and frame layout to insert stack
adjustment, callee-save handling, frame/base-pointer setup, variadic entry work,
returns, and target-required unwind state.

Legacy coverage: target prologue/returns, frame emission, callee saves, dynamic
stack restoration, variadic entry, and exceptional/early return paths.
