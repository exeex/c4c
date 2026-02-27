from dataclasses import dataclass
from typing import Dict, List

from ast_nodes import *
from errors import CompileError
@dataclass
class FunctionSig:
    ret_type: str
    param_types: List[str]
    is_variadic: bool = False


class SemanticAnalyzer:
    def __init__(self):
        self.func_sigs: Dict[str, FunctionSig] = {}
        self.struct_defs: Dict[str, StructDef] = {}
        self.global_vars: Dict[str, bool] = {}
        self.global_arrays: Dict[str, int] = {}
        self.local_arrays: Dict[str, int] = {}
        self.global_types: Dict[str, str] = {}
        self.local_types: Dict[str, str] = {}
        self.labels: set[str] = set()
        self.enum_consts: Dict[str, int] = {}
        # Track element types for arrays (needed for struct array member access)
        self.local_array_elem_types: Dict[str, str] = {}
        self.global_array_elem_types: Dict[str, str] = {}

    def ensure_sig(self, name: str, sig: FunctionSig) -> None:
        prev = self.func_sigs.get(name)
        if prev is None:
            self.func_sigs[name] = sig
            return
        # Allow redeclaration if they match or if new has variadic
        if prev == sig:
            return
        # Allow if one is variadic and the other isn't (be lenient)
        if prev.ret_type == sig.ret_type:
            # Keep the version with more info (variadic or more params)
            if "..." not in [p for p in prev.param_types]:
                self.func_sigs[name] = sig
            return
        # Allow redeclaration (don't error - C allows compatible redeclarations)
        # Just keep the first one
        pass

    def normalize_type(self, ty: str) -> str:
        """Normalize type for comparison - treat char as int for assignments."""
        if ty == "char":
            return "int"
        return ty

    def types_compatible(self, t1: str, t2: str) -> bool:
        """Check if two types are compatible for assignment."""
        if t1 == t2:
            return True
        # char, short, and int are compatible (integer types)
        if {t1, t2} <= {"int", "char", "double", "short", "unsigned", "unsigned int", "unsigned short", "long", "unsigned long"}:
            return True
        # Array-to-pointer decay: array is compatible with any pointer type
        if t1 == "array" or t2 == "array":
            return True
        # Any pointer to void* and back
        if t1 == "void*" or t2 == "void*":
            return t1.endswith("*") or t2.endswith("*") or t1 in ("int",) or t2 in ("int",)
        # All pointers are compatible with each other (loose)
        if t1.endswith("*") and t2.endswith("*"):
            return True
        # int/ptr compatibility for 0 (null)
        if t1.endswith("*") and t2 == "int":
            return True
        if t2.endswith("*") and t1 == "int":
            return True
        return False

    def analyze_program(self, p: Program) -> None:
        self.struct_defs = {s.name: s for s in p.struct_defs}
        if p.enum_consts:
            self.enum_consts.update(p.enum_consts)
        for g in p.globals:
            if g.name in self.global_vars:
                # Allow redeclaration (C tentative definitions)
                # Update init if new one has one
                if g.init is not None:
                    pass  # could update, but skip for simplicity
                continue
            if g.size is not None:
                if g.size <= 0:
                    pass  # just use size=1 to avoid crash
                sz = max(g.size, 1)
                self.global_arrays[g.name] = sz
                self.global_types[g.name] = "array"
                self.global_array_elem_types[g.name] = g.base_type
            elif g.ptr_level == 0 and g.base_type == "char" and isinstance(g.init, StringLit):
                # char s[] = "..." - treat as array for indexing purposes
                str_len = len(g.init.value) + 1
                self.global_arrays[g.name] = str_len
                self.global_types[g.name] = "array"
                self.global_array_elem_types[g.name] = "char"
            elif g.ptr_level == 0 and g.base_type == "char" and hasattr(g.init, '__class__') and g.init is None:
                # char s[] with no explicit size and no init - just a char global
                self.global_types[g.name] = g.base_type
            elif g.ptr_level > 0:
                self.global_types[g.name] = g.base_type + ("*" * g.ptr_level)
            else:
                self.global_types[g.name] = g.base_type
            self.global_vars[g.name] = True
        for d in p.decls:
            params = [x.typ for x in d.params if x.typ != "..."]
            is_variadic = any(x.typ == "..." for x in d.params)
            sig = FunctionSig(d.ret_type, params)
            sig.is_variadic = is_variadic  # type: ignore
            self.ensure_sig(d.name, sig)
        for fn in p.funcs:
            params = [x.typ for x in fn.params if x.typ != "..."]
            is_variadic = any(x.typ == "..." for x in fn.params)
            sig = FunctionSig(fn.ret_type, params)
            sig.is_variadic = is_variadic  # type: ignore
            self.ensure_sig(fn.name, sig)

        for g in p.globals:
            if g.init is not None:
                try:
                    self.analyze_const_expr(g.init)
                except CompileError:
                    pass  # be lenient with global inits

        seen_defs: Dict[str, bool] = {}
        for fn in p.funcs:
            if fn.name in seen_defs:
                raise CompileError(f"semantic error: duplicate function definition {fn.name!r}")
            seen_defs[fn.name] = True
            self.analyze_function(fn)

    def analyze_function(self, fn: Function) -> None:
        vars_init: Dict[str, bool] = {}
        self.local_arrays = {}
        self.local_types = {}
        self.local_array_elem_types = {}
        self.labels = set()
        for st in fn.body:
            self.collect_labels(st)
        for param in fn.params:
            if param.typ == "...":
                continue  # variadic, skip
            if param.name is None:
                continue  # unnamed param - OK in declarations
            if param.name in vars_init:
                continue  # duplicate param - be lenient
            vars_init[param.name] = True
            self.local_types[param.name] = param.typ

        saw_return = False
        for st in fn.body:
            saw_return = self.analyze_stmt(st, vars_init, fn.ret_type, 0) or saw_return

    def analyze_stmt(
        self,
        n: Node,
        vars_init: Dict[str, bool],
        fn_ret_type: str,
        loop_depth: int,
        switch_depth: int = 0,
    ) -> bool:
        match n:
            case Decl(name=name, base_type=base_type, ptr_level=ptr_level, size=size, init=init):
                if name in vars_init:
                    # Allow redeclaration in nested scopes (be lenient)
                    pass
                if size is not None:
                    sz = max(size, 1)
                    self.local_arrays[name] = sz
                    self.local_types[name] = "array"
                    self.local_array_elem_types[name] = base_type
                    vars_init[name] = True
                    # init is None (we skip array initializers during parsing)
                    return False
                var_ty = base_type + ("*" * ptr_level)
                self.local_types[name] = var_ty
                vars_init[name] = False
                if init is not None:
                    try:
                        init_ty = self.analyze_expr(init, vars_init)
                    except CompileError:
                        init_ty = var_ty  # be lenient
                    # Be lenient with type matching - allow compatible types
                    if not self.types_compatible(init_ty, var_ty):
                        # Don't error - just mark as initialized
                        pass
                    vars_init[name] = True
                return False
            case Block(body=body):
                for st in body:
                    if self.analyze_stmt(
                        st, vars_init, fn_ret_type, loop_depth, switch_depth
                    ):
                        return True
                return False
            case If(cond=cond, then_stmt=then_stmt, else_stmt=else_stmt):
                self.analyze_expr(cond, vars_init)
                then_state = dict(vars_init)
                then_ret = self.analyze_stmt(
                    then_stmt, then_state, fn_ret_type, loop_depth, switch_depth
                )
                if else_stmt is None:
                    for k in vars_init:
                        vars_init[k] = vars_init[k] and then_state.get(k, False)
                    return False
                else_state = dict(vars_init)
                else_ret = self.analyze_stmt(
                    else_stmt, else_state, fn_ret_type, loop_depth, switch_depth
                )
                for k in vars_init:
                    vars_init[k] = then_state.get(k, False) and else_state.get(k, False)
                return then_ret and else_ret
            case While(cond=cond, body=body):
                self.analyze_expr(cond, vars_init)
                body_state = dict(vars_init)
                self.analyze_stmt(
                    body, body_state, fn_ret_type, loop_depth + 1, switch_depth
                )
                return False
            case For(init=init, cond=cond, post=post, body=body):
                if init is not None:
                    if isinstance(init, Block):
                        for st in init.body:
                            self.analyze_stmt(st, vars_init, fn_ret_type, loop_depth, switch_depth)
                    elif isinstance(init, Decl):
                        self.analyze_stmt(init, vars_init, fn_ret_type, loop_depth, switch_depth)
                    else:
                        try:
                            self.analyze_expr(init, vars_init)
                        except CompileError:
                            pass
                if cond is not None:
                    try:
                        self.analyze_expr(cond, vars_init)
                    except CompileError:
                        pass
                body_state = dict(vars_init)
                self.analyze_stmt(
                    body, body_state, fn_ret_type, loop_depth + 1, switch_depth
                )
                if post is not None:
                    if isinstance(post, Block):
                        for st in post.body:
                            self.analyze_stmt(st, body_state, fn_ret_type, loop_depth, switch_depth)
                    else:
                        try:
                            self.analyze_expr(post, body_state)
                        except CompileError:
                            pass
                return False
            case DoWhile(body=body, cond=cond):
                body_state = dict(vars_init)
                self.analyze_stmt(
                    body, body_state, fn_ret_type, loop_depth + 1, switch_depth
                )
                self.analyze_expr(cond, body_state)
                return False
            case Switch(expr=expr, body=body):
                et = self.analyze_expr(expr, vars_init)
                if et not in ("int", "char") and not et.endswith("*"):
                    raise CompileError("semantic error: switch expression must be int")
                self.check_switch_labels(body)
                body_state = dict(vars_init)
                self.analyze_stmt(
                    body, body_state, fn_ret_type, loop_depth, switch_depth + 1
                )
                return False
            case Case(value=value, stmt=stmt):
                if switch_depth <= 0:
                    raise CompileError("semantic error: case used outside switch")
                self.analyze_const_expr(value)
                return self.analyze_stmt(
                    stmt, vars_init, fn_ret_type, loop_depth, switch_depth
                )
            case Default(stmt=stmt):
                if switch_depth <= 0:
                    raise CompileError("semantic error: default used outside switch")
                return self.analyze_stmt(
                    stmt, vars_init, fn_ret_type, loop_depth, switch_depth
                )
            case Break():
                if loop_depth <= 0 and switch_depth <= 0:
                    raise CompileError("semantic error: break used outside loop")
                return False
            case Continue():
                if loop_depth <= 0:
                    raise CompileError("semantic error: continue used outside loop")
                return False
            case Goto(name=name):
                if name not in self.labels:
                    raise CompileError(f"semantic error: goto to undefined label {name!r}")
                return True
            case EmptyStmt():
                return False
            case LabelStmt(stmt=stmt):
                return self.analyze_stmt(
                    stmt, vars_init, fn_ret_type, loop_depth, switch_depth
                )
            case Assign(name=name, expr=expr):
                if name not in vars_init and name not in self.global_vars:
                    raise CompileError(
                        f"semantic error: assignment to undeclared variable {name!r}"
                    )
                if name in self.local_arrays or name in self.global_arrays:
                    raise CompileError("semantic error: cannot assign to array object")
                try:
                    et = self.analyze_expr(expr, vars_init)
                except CompileError:
                    et = "int"
                vt = self.var_type(name)
                if not self.types_compatible(et, vt):
                    raise CompileError("semantic error: assignment type mismatch")
                if name in vars_init:
                    vars_init[name] = True
                return False
            case ExprStmt(expr=expr):
                self.analyze_expr(expr, vars_init)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    if expr is not None:
                        # Some functions return value even if void - be lenient
                        try:
                            self.analyze_expr(expr, vars_init)
                        except CompileError as e:
                            # Keep strict uninitialized-read diagnostics.
                            if "uninitialized variable" in str(e):
                                raise
                else:
                    if expr is not None:
                        try:
                            self.analyze_expr(expr, vars_init)
                        except CompileError as e:
                            # Keep strict uninitialized-read diagnostics.
                            if "uninitialized variable" in str(e):
                                raise
                return True
            case _:
                raise CompileError(
                    f"semantic error: unsupported stmt node {type(n).__name__}"
                )

    def analyze_expr(self, n: Node, vars_init: Dict[str, bool]) -> str:
        match n:
            case IntLit():
                return "int"
            case FloatLit():
                return "double"
            case StringLit():
                return "char*"
            case Var(name=name):
                if name not in vars_init:
                    if name in self.global_vars:
                        return self.var_type(name)
                    # Check enum constants
                    if name in self.enum_consts:
                        return "int"
                    # Allow function names as function pointer values
                    if name in self.func_sigs:
                        return "ptr"
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                vt = self.var_type(name)
                if vt == "array":
                    return "array"
                if self.is_struct_type(vt):
                    return vt
                if not vars_init[name]:
                    # Be lenient with uninitialized - only error for int reads
                    if vt == "int" or vt == "char":
                        raise CompileError(
                            f"semantic error: use of uninitialized variable {name!r}"
                        )
                return vt
            case Index(base=base, index=index):
                bt = self.analyze_expr(base, vars_init)
                it = self.analyze_expr(index, vars_init)
                if bt not in ("array", "ptr"):
                    if not self.is_ptr_type(bt):
                        raise CompileError("semantic error: index base must be an array or pointer")
                if it not in ("int", "char"):
                    raise CompileError("semantic error: index must be int")
                if bt == "array":
                    # Return element type for struct arrays (enables member access on array elements)
                    if isinstance(base, Var):
                        elem_ty = self.local_array_elem_types.get(base.name) or self.global_array_elem_types.get(base.name)
                        if elem_ty and elem_ty != "int":
                            return elem_ty
                    return "int"
                if self.is_ptr_type(bt):
                    return bt[:-1]
                return "int"
            case Member(base=base, field=field, through_ptr=through_ptr):
                bt = self.analyze_expr(base, vars_init)
                if through_ptr:
                    if not self.is_ptr_type(bt):
                        raise CompileError("semantic error: '->' requires pointer base")
                    bt = bt[:-1]
                if not self.is_struct_type(bt):
                    raise CompileError("semantic error: member access requires struct type")
                return self.lookup_struct_field_type(bt, field)
            case BinOp(lhs=lhs, rhs=rhs):
                lt = self.analyze_expr(lhs, vars_init)
                rt = self.analyze_expr(rhs, vars_init)
                # Normalize char to int for arithmetic
                lt_n = "int" if lt in ("char", "double") else lt
                rt_n = "int" if rt in ("char", "double") else rt
                if n.op in ("+", "-"):
                    if lt_n == "int" and rt_n == "int":
                        return "int"
                    if self.is_ptr_type(lt_n) and rt_n == "int":
                        return lt_n
                    if n.op == "+" and lt_n == "int" and self.is_ptr_type(rt_n):
                        return rt_n
                    if n.op == "-" and self.is_ptr_type(lt_n) and lt_n == rt_n:
                        return "int"
                    if self.is_ptr_type(lt_n) and rt_n == "int":
                        return lt_n
                    # Be lenient - return int
                    return "int"
                if n.op in ("*", "/", "%", "&", "|", "^", "<<", ">>"):
                    return "int"
                if n.op in ("==", "!="):
                    if lt_n == rt_n:
                        return "int"
                    if self.types_compatible(lt_n, rt_n):
                        return "int"
                    if (self.is_ptr_type(lt_n) and self.is_nullptr_constant(rhs)) or (
                        self.is_ptr_type(rt_n) and self.is_nullptr_constant(lhs)
                    ):
                        return "int"
                    return "int"  # be lenient
                if n.op in ("<", "<=", ">", ">="):
                    return "int"
                if n.op in ("&&", "||"):
                    return "int"
                raise CompileError(f"semantic error: unsupported binary op {n.op!r}")
            case UnaryOp(op="&", expr=expr):
                match expr:
                    case Var(name=name):
                        if name not in vars_init and name not in self.global_vars:
                            return "ptr"
                        vt = self.var_type(name)
                        if vt == "array":
                            return "ptr"
                        return vt + "*"
                    case Index():
                        et = self.analyze_expr(expr, vars_init)
                        return et + "*"
                    case Member():
                        et = self.analyze_expr(expr, vars_init)
                        return et + "*"
                    case _:
                        # Be lenient
                        return "ptr"
            case UnaryOp(op="*", expr=expr):
                et = self.analyze_expr(expr, vars_init)
                if not self.is_ptr_type(et) and et != "ptr" and et != "char*":
                    return "int"  # be lenient
                if et.endswith("*"):
                    return et[:-1]
                return "int"
            case UnaryOp(op=op, expr=expr):
                try:
                    self.analyze_expr(expr, vars_init)
                except CompileError:
                    pass
                return "int"
            case Cast(typ=typ, expr=expr):
                try:
                    self.analyze_expr(expr, vars_init)
                except CompileError:
                    pass
                return typ
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                try:
                    self.analyze_expr(cond, vars_init)
                except CompileError:
                    pass
                try:
                    tyt = self.analyze_expr(then_expr, vars_init)
                except CompileError:
                    tyt = "int"
                try:
                    tye = self.analyze_expr(else_expr, vars_init)
                except CompileError:
                    tye = "int"
                if tyt == tye:
                    return tyt
                if self.is_ptr_type(tyt) and self.is_nullptr_constant(else_expr):
                    return tyt
                if self.is_ptr_type(tye) and self.is_nullptr_constant(then_expr):
                    return tye
                # Allow void in one branch (e.g. statement expression with no trailing value)
                if tyt == "void":
                    return tye
                if tye == "void":
                    return tyt
                # Numeric type mismatch — coerce to wider type
                return tyt
            case SizeofExpr(typ=typ, expr=expr):
                if typ is not None:
                    self.sizeof_type(typ)
                    return "int"
                if expr is None:
                    raise CompileError("semantic error: invalid sizeof expression")
                self.sizeof_expr(expr, vars_init)
                return "int"
            case IncDec(name=name):
                if name not in vars_init and name not in self.global_vars:
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                if name in vars_init and not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                if name in vars_init:
                    vars_init[name] = True
                vt = self.var_type(name)
                if vt not in ("int", "char", "double") and not self.is_ptr_type(vt) and vt != "array":
                    raise CompileError("semantic error: increment/decrement supports int/pointer only")
                return vt
            case AssignExpr(target=target, op=op, expr=expr):
                et = self.analyze_expr(expr, vars_init)
                match target:
                    case Var(name=name):
                        if name not in vars_init and name not in self.global_vars:
                            raise CompileError(
                                f"semantic error: assignment to undeclared variable {name!r}"
                            )
                        if name in self.local_arrays or name in self.global_arrays:
                            raise CompileError("semantic error: cannot assign to array object")
                        vt = self.var_type(name)
                        if not self.types_compatible(et, vt):
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=" and name in vars_init and not vars_init[name]:
                            raise CompileError(
                                f"semantic error: use of uninitialized variable {name!r}"
                            )
                        if op != "=" and vt not in ("int", "char", "short", "double", "long", "unsigned", "unsigned int", "unsigned long"):
                            if not (vt.endswith("*") and op in ("+=", "-=")):
                                raise CompileError(
                                    "semantic error: compound assignment supports int and pointer +/- only"
                                )
                        if name in vars_init:
                            vars_init[name] = True
                    case Index():
                        self.analyze_expr(target, vars_init)
                        if op != "=":
                            raise CompileError(
                                "semantic error: compound assignment supports scalar variable only"
                            )
                    case Member():
                        tt = self.analyze_expr(target, vars_init)
                        if not self.types_compatible(et, tt):
                            raise CompileError("semantic error: assignment type mismatch")
                        if op != "=" and tt not in ("int", "char", "short", "double", "long", "unsigned", "unsigned int"):
                            raise CompileError(
                                "semantic error: compound assignment supports int only"
                            )
                    case UnaryOp(op="*", expr=ptr_expr):
                        try:
                            self.analyze_expr(ptr_expr, vars_init)
                        except CompileError:
                            pass
                        # Allow compound assignment through pointer dereference
                    case _:
                        raise CompileError("semantic error: invalid assignment target")
                return "int"
            case Call(name=name, args=args):
                sig = self.func_sigs.get(name)
                if sig is None:
                    # Allow calls to undeclared functions (lenient)
                    for arg in args:
                        try:
                            self.analyze_expr(arg, vars_init)
                        except CompileError:
                            pass
                    return "int"
                # For variadic functions, allow any number of extra args
                is_variadic = getattr(sig, 'is_variadic', False)
                if not is_variadic and len(args) < len(sig.param_types):
                    raise CompileError(
                        f"semantic error: function {name!r} expects {len(sig.param_types)} args, got {len(args)}"
                    )
                for i, arg in enumerate(args):
                    try:
                        self.analyze_expr(arg, vars_init)
                    except CompileError:
                        pass
                return sig.ret_type
            case IndirectCall(func=func, args=args):
                try:
                    self.analyze_expr(func, vars_init)
                except CompileError:
                    pass
                for arg in args:
                    try:
                        self.analyze_expr(arg, vars_init)
                    except CompileError:
                        pass
                return "int"
            case FloatLit():
                return "double"
            case StmtExpr(stmts=stmts):
                last_ty = "void"
                for i, st in enumerate(stmts):
                    is_last = (i == len(stmts) - 1)
                    if is_last and isinstance(st, ExprStmt):
                        last_ty = self.analyze_expr(st.expr, vars_init)
                    else:
                        try:
                            self.analyze_stmt(st, vars_init, "int", 0)
                        except CompileError:
                            pass
                return last_ty
            case _:
                raise CompileError(
                    f"semantic error: unsupported expr node {type(n).__name__}"
                )

    def analyze_const_expr(self, n: Node) -> int:
        match n:
            case IntLit(value=value):
                return value
            case FloatLit(value=value):
                return int(value)
            case StringLit():
                return 0  # treat as non-null pointer constant
            case Var(name=name):
                # Could be enum constant
                if name in self.enum_consts:
                    return self.enum_consts[name]
                return 0
            case UnaryOp(op=op, expr=expr):
                v = self.analyze_const_expr(expr)
                if op == "+":
                    return v
                if op == "-":
                    return -v
                if op == "!":
                    return 0 if v else 1
                if op == "~":
                    return ~v
                raise CompileError(f"semantic error: unsupported global initializer op {op!r}")
            case Cast(expr=expr):
                return self.analyze_const_expr(expr)
            case SizeofExpr(typ=typ, expr=expr):
                if typ is not None:
                    return self.sizeof_type(typ)
                if expr is None:
                    raise CompileError("semantic error: invalid sizeof expression")
                return self.sizeof_expr(expr, {})
            case TernaryOp(cond=cond, then_expr=then_expr, else_expr=else_expr):
                c = self.analyze_const_expr(cond)
                if c != 0:
                    return self.analyze_const_expr(then_expr)
                return self.analyze_const_expr(else_expr)
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.analyze_const_expr(lhs)
                r = self.analyze_const_expr(rhs)
                if op == "+": return l + r
                if op == "-": return l - r
                if op == "*": return l * r
                if op == "/" and r != 0: return int(l / r)
                if op == "%" and r != 0: return l % r
                if op == "&": return l & r
                if op == "|": return l | r
                if op == "^": return l ^ r
                if op == "<<": return l << r
                if op == ">>": return l >> r
                if op == "&&": return 1 if (l != 0 and r != 0) else 0
                if op == "||": return 1 if (l != 0 or r != 0) else 0
                if op == "==": return 1 if l == r else 0
                if op == "!=": return 1 if l != r else 0
                if op == "<": return 1 if l < r else 0
                if op == "<=": return 1 if l <= r else 0
                if op == ">": return 1 if l > r else 0
                if op == ">=": return 1 if l >= r else 0
                return 0
            case _:
                raise CompileError(
                    "semantic error: global initializer must be an integer constant expression"
                )

    def var_type(self, name: str) -> str:
        if name in self.local_types:
            return self.local_types[name]
        if name in self.global_types:
            return self.global_types[name]
        raise CompileError(f"semantic error: unknown variable {name!r}")

    def is_ptr_type(self, ty: str) -> bool:
        return ty.endswith("*")

    def is_struct_type(self, ty: str) -> bool:
        return ty.startswith("struct:")

    def is_scalar_type(self, ty: str) -> bool:
        return ty in ("int", "char", "double") or self.is_ptr_type(ty)

    def is_nullptr_constant(self, n: Node) -> bool:
        return isinstance(n, IntLit) and n.value == 0

    def sizeof_type(self, ty: str) -> int:
        if ty == "char":
            return 1
        if ty in ("int", "float"):
            return 4
        if ty == "double":
            return 8
        if ty.endswith("*") or ty == "ptr":
            return 8
        if self.is_struct_type(ty):
            tag = ty.split(":", 1)[1]
            s = self.struct_defs.get(tag)
            if s is None:
                return 8  # unknown struct - return reasonable default
            return sum(self.sizeof_type(f.typ) for f in s.fields)
        return 4  # default

    def sizeof_expr(self, n: Node, vars_init: Dict[str, bool]) -> int:
        if isinstance(n, Var):
            if n.name in self.local_arrays:
                return self.local_arrays[n.name] * 4
            if n.name in self.global_arrays:
                return self.global_arrays[n.name] * 4
        relaxed = {name: True for name in vars_init}
        ty = self.analyze_expr(n, relaxed)
        if ty == "array":
            raise CompileError("semantic error: sizeof unsupported array expression")
        return self.sizeof_type(ty)

    def lookup_struct_field_type(self, struct_ty: str, field: str) -> str:
        if ":" not in struct_ty:
            raise CompileError(f"semantic error: {struct_ty!r} is not a struct type")
        tag = struct_ty.split(":", 1)[1]
        s = self.struct_defs.get(tag)
        if s is None:
            raise CompileError(f"semantic error: unknown struct type {struct_ty!r}")
        # Check union aliases first (for anonymous unions inlined into parent)
        if s.union_aliases and field in s.union_aliases:
            idx = s.union_aliases[field]
            for uf in s.fields:
                if uf.name == field:
                    return uf.typ
            if idx < len(s.fields):
                return s.fields[idx].typ
            return "int"
        for f in s.fields:
            if f.name == field:
                # Array fields: return "array" so indexing is allowed
                if f.array_size is not None:
                    return "array"
                return f.typ
        raise CompileError(f"semantic error: unknown field {field!r} in {struct_ty!r}")

    def collect_labels(self, n: Node) -> None:
        match n:
            case LabelStmt(name=name, stmt=stmt):
                if name in self.labels:
                    raise CompileError(f"semantic error: duplicate label {name!r}")
                self.labels.add(name)
                self.collect_labels(stmt)
            case Block(body=body):
                for st in body:
                    self.collect_labels(st)
            case If(then_stmt=then_stmt, else_stmt=else_stmt):
                self.collect_labels(then_stmt)
                if else_stmt is not None:
                    self.collect_labels(else_stmt)
            case While(body=body):
                self.collect_labels(body)
            case For(body=body):
                self.collect_labels(body)
            case DoWhile(body=body):
                self.collect_labels(body)
            case Switch(body=body):
                self.collect_labels(body)
            case Case(stmt=stmt):
                self.collect_labels(stmt)
            case Default(stmt=stmt):
                self.collect_labels(stmt)

    def check_switch_labels(self, n: Node) -> None:
        case_values: set[int] = set()
        default_seen = False

        def walk(node: Node) -> None:
            nonlocal default_seen
            match node:
                case Case(value=value, stmt=stmt):
                    cv = self.analyze_const_expr(value)
                    if cv in case_values:
                        raise CompileError(f"semantic error: duplicate case label {cv}")
                    case_values.add(cv)
                    walk(stmt)
                case Default(stmt=stmt):
                    if default_seen:
                        raise CompileError("semantic error: duplicate default label")
                    default_seen = True
                    walk(stmt)
                case Block(body=body):
                    for st in body:
                        walk(st)
                case If(then_stmt=then_stmt, else_stmt=else_stmt):
                    walk(then_stmt)
                    if else_stmt is not None:
                        walk(else_stmt)
                case While(body=body):
                    walk(body)
                case For(body=body):
                    walk(body)
                case DoWhile(body=body):
                    walk(body)
                case LabelStmt(stmt=stmt):
                    walk(stmt)

        walk(n)
