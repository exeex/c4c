#!/usr/bin/env python3
import argparse
import sys
from dataclasses import dataclass
from enum import Enum, auto
from typing import Dict, List, Optional


class CompileError(Exception):
    pass


class TokenType(Enum):
    KW_INT = auto()
    KW_VOID = auto()
    KW_EXTERN = auto()
    KW_RETURN = auto()
    ID = auto()
    NUM = auto()
    LBRACE = auto()
    RBRACE = auto()
    LPAREN = auto()
    RPAREN = auto()
    SEMI = auto()
    COMMA = auto()
    ASSIGN = auto()
    PLUS = auto()
    MINUS = auto()
    STAR = auto()
    SLASH = auto()
    EOF = auto()


@dataclass(frozen=True)
class Token:
    typ: TokenType
    text: str
    line: int
    col: int


SINGLE_CHAR_TOKENS = {
    "{": TokenType.LBRACE,
    "}": TokenType.RBRACE,
    "(": TokenType.LPAREN,
    ")": TokenType.RPAREN,
    ";": TokenType.SEMI,
    ",": TokenType.COMMA,
    "=": TokenType.ASSIGN,
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
    "*": TokenType.STAR,
    "/": TokenType.SLASH,
}


class Lexer:
    def __init__(self, src: str):
        self.src = src
        self.i = 0
        self.line = 1
        self.col = 1

    def cur(self) -> str:
        if self.i >= len(self.src):
            return ""
        return self.src[self.i]

    def peek(self, n: int = 1) -> str:
        j = self.i + n
        if j >= len(self.src):
            return ""
        return self.src[j]

    def advance(self) -> str:
        ch = self.cur()
        if not ch:
            return ""
        self.i += 1
        if ch == "\n":
            self.line += 1
            self.col = 1
        else:
            self.col += 1
        return ch

    def scan_identifier(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = [self.advance()]
        while True:
            ch = self.cur()
            if ch.isalnum() or ch == "_":
                buf.append(self.advance())
            else:
                break
        text = "".join(buf)
        kw_map = {
            "int": TokenType.KW_INT,
            "void": TokenType.KW_VOID,
            "extern": TokenType.KW_EXTERN,
            "return": TokenType.KW_RETURN,
        }
        return Token(kw_map.get(text, TokenType.ID), text, start_line, start_col)

    def scan_number(self) -> Token:
        start_line = self.line
        start_col = self.col
        buf = [self.advance()]
        while self.cur().isdigit():
            buf.append(self.advance())
        return Token(TokenType.NUM, "".join(buf), start_line, start_col)

    def scan(self) -> List[Token]:
        tokens: List[Token] = []
        while True:
            ch = self.cur()
            if not ch:
                break

            if ch.isspace():
                self.advance()
                continue

            if ch == "/" and self.peek() == "/":
                while self.cur() and self.cur() != "\n":
                    self.advance()
                continue

            if ch == "/" and self.peek() == "*":
                start_line, start_col = self.line, self.col
                self.advance()
                self.advance()
                while True:
                    if not self.cur():
                        raise CompileError(
                            f"{start_line}:{start_col}: unterminated block comment"
                        )
                    if self.cur() == "*" and self.peek() == "/":
                        self.advance()
                        self.advance()
                        break
                    self.advance()
                continue

            if ch.isalpha() or ch == "_":
                tokens.append(self.scan_identifier())
                continue

            if ch.isdigit():
                tokens.append(self.scan_number())
                continue

            tok_type = SINGLE_CHAR_TOKENS.get(ch)
            if tok_type is not None:
                tokens.append(Token(tok_type, ch, self.line, self.col))
                self.advance()
                continue

            raise CompileError(f"{self.line}:{self.col}: unexpected character {ch!r}")

        tokens.append(Token(TokenType.EOF, "", self.line, self.col))
        return tokens


class Node:
    pass


@dataclass
class Param(Node):
    typ: str
    name: Optional[str]


@dataclass
class FunctionDecl(Node):
    ret_type: str
    name: str
    params: List[Param]


@dataclass
class Function(Node):
    ret_type: str
    name: str
    params: List[Param]
    body: List[Node]


@dataclass
class Program(Node):
    decls: List[FunctionDecl]
    funcs: List[Function]


@dataclass
class Decl(Node):
    name: str
    init: Optional[Node]


@dataclass
class Assign(Node):
    name: str
    expr: Node


@dataclass
class ExprStmt(Node):
    expr: Node


@dataclass
class Return(Node):
    expr: Optional[Node]


@dataclass
class IntLit(Node):
    value: int


@dataclass
class Var(Node):
    name: str


@dataclass
class Call(Node):
    name: str
    args: List[Node]


@dataclass
class BinOp(Node):
    op: str
    lhs: Node
    rhs: Node


class Parser:
    def __init__(self, tokens: List[Token]):
        self.tokens = tokens
        self.i = 0

    def cur(self) -> Token:
        return self.tokens[self.i]

    def peek(self, n: int = 1) -> Token:
        j = self.i + n
        if j >= len(self.tokens):
            return self.tokens[-1]
        return self.tokens[j]

    def advance(self) -> None:
        self.i += 1

    def error(self, msg: str) -> CompileError:
        t = self.cur()
        return CompileError(f"{t.line}:{t.col}: {msg} (got {t.text!r})")

    def eat(self, typ: TokenType) -> Token:
        t = self.cur()
        if t.typ != typ:
            raise self.error(f"expected {typ.name}")
        self.advance()
        return t

    def parse_type(self) -> str:
        match self.cur().typ:
            case TokenType.KW_INT:
                self.advance()
                return "int"
            case TokenType.KW_VOID:
                self.advance()
                return "void"
            case _:
                raise self.error("expected type")

    def parse_params(self) -> List[Param]:
        if self.cur().typ == TokenType.RPAREN:
            return []

        if self.cur().typ == TokenType.KW_VOID and self.peek().typ == TokenType.RPAREN:
            self.advance()
            return []

        params: List[Param] = []
        while True:
            typ = self.parse_type()
            name: Optional[str] = None
            if self.cur().typ == TokenType.ID:
                name = self.eat(TokenType.ID).text
            params.append(Param(typ, name))
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return params

    def parse_program(self) -> Program:
        decls: List[FunctionDecl] = []
        funcs: List[Function] = []
        while self.cur().typ != TokenType.EOF:
            ext = self.parse_external()
            if isinstance(ext, FunctionDecl):
                decls.append(ext)
            else:
                funcs.append(ext)
        self.eat(TokenType.EOF)
        return Program(decls, funcs)

    def parse_external(self) -> FunctionDecl | Function:
        if self.cur().typ == TokenType.KW_EXTERN:
            self.advance()
        ret_type = self.parse_type()
        name = self.eat(TokenType.ID).text
        self.eat(TokenType.LPAREN)
        params = self.parse_params()
        self.eat(TokenType.RPAREN)

        if self.cur().typ == TokenType.SEMI:
            self.advance()
            return FunctionDecl(ret_type, name, params)

        self.eat(TokenType.LBRACE)
        body: List[Node] = []
        while self.cur().typ != TokenType.RBRACE:
            body.append(self.parse_stmt())
        self.eat(TokenType.RBRACE)
        return Function(ret_type, name, params, body)

    def parse_stmt(self) -> Node:
        match self.cur().typ:
            case TokenType.KW_INT:
                self.advance()
                name = self.eat(TokenType.ID).text
                init: Optional[Node] = None
                if self.cur().typ == TokenType.ASSIGN:
                    self.advance()
                    init = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Decl(name, init)
            case TokenType.KW_RETURN:
                self.advance()
                expr: Optional[Node] = None
                if self.cur().typ != TokenType.SEMI:
                    expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Return(expr)
            case TokenType.ID if self.peek().typ == TokenType.ASSIGN:
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.ASSIGN)
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Assign(name, expr)
            case _:
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return ExprStmt(expr)

    def parse_expr(self) -> Node:
        node = self.parse_term()
        while self.cur().typ in (TokenType.PLUS, TokenType.MINUS):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_term())
        return node

    def parse_term(self) -> Node:
        node = self.parse_factor()
        while self.cur().typ in (TokenType.STAR, TokenType.SLASH):
            op = self.cur().text
            self.advance()
            node = BinOp(op, node, self.parse_factor())
        return node

    def parse_args(self) -> List[Node]:
        if self.cur().typ == TokenType.RPAREN:
            return []
        args: List[Node] = []
        while True:
            args.append(self.parse_expr())
            if self.cur().typ != TokenType.COMMA:
                break
            self.advance()
        return args

    def parse_factor(self) -> Node:
        match self.cur().typ:
            case TokenType.NUM:
                return IntLit(int(self.eat(TokenType.NUM).text))
            case TokenType.ID:
                name = self.eat(TokenType.ID).text
                if self.cur().typ == TokenType.LPAREN:
                    self.advance()
                    args = self.parse_args()
                    self.eat(TokenType.RPAREN)
                    return Call(name, args)
                return Var(name)
            case TokenType.LPAREN:
                self.advance()
                n = self.parse_expr()
                self.eat(TokenType.RPAREN)
                return n
            case _:
                raise self.error("expected expression")


@dataclass
class FunctionSig:
    ret_type: str
    param_types: List[str]


class SemanticAnalyzer:
    def __init__(self):
        self.func_sigs: Dict[str, FunctionSig] = {}

    def ensure_sig(self, name: str, sig: FunctionSig) -> None:
        prev = self.func_sigs.get(name)
        if prev is None:
            self.func_sigs[name] = sig
            return
        if prev != sig:
            raise CompileError(f"semantic error: conflicting declaration for function {name!r}")

    def analyze_program(self, p: Program) -> None:
        for d in p.decls:
            self.ensure_sig(d.name, FunctionSig(d.ret_type, [x.typ for x in d.params]))
        for fn in p.funcs:
            self.ensure_sig(fn.name, FunctionSig(fn.ret_type, [x.typ for x in fn.params]))

        seen_defs: Dict[str, bool] = {}
        for fn in p.funcs:
            if fn.name in seen_defs:
                raise CompileError(f"semantic error: duplicate function definition {fn.name!r}")
            seen_defs[fn.name] = True
            self.analyze_function(fn)

    def analyze_function(self, fn: Function) -> None:
        vars_init: Dict[str, bool] = {}
        for param in fn.params:
            if param.name is None:
                raise CompileError(
                    f"semantic error: parameter name required in function definition {fn.name!r}"
                )
            if param.name in vars_init:
                raise CompileError(
                    f"semantic error: duplicate parameter name {param.name!r} in {fn.name!r}"
                )
            vars_init[param.name] = True

        saw_return = False
        for st in fn.body:
            saw_return = self.analyze_stmt(st, vars_init, fn.ret_type) or saw_return
            if saw_return:
                break

    def analyze_stmt(self, n: Node, vars_init: Dict[str, bool], fn_ret_type: str) -> bool:
        match n:
            case Decl(name=name, init=init):
                if name in vars_init:
                    raise CompileError(f"semantic error: variable redeclared: {name!r}")
                vars_init[name] = False
                if init is not None:
                    self.analyze_expr(init, vars_init)
                    vars_init[name] = True
                return False
            case Assign(name=name, expr=expr):
                if name not in vars_init:
                    raise CompileError(
                        f"semantic error: assignment to undeclared variable {name!r}"
                    )
                self.analyze_expr(expr, vars_init)
                vars_init[name] = True
                return False
            case ExprStmt(expr=expr):
                self.analyze_expr(expr, vars_init)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    if expr is not None:
                        raise CompileError("semantic error: void function cannot return a value")
                else:
                    if expr is None:
                        raise CompileError("semantic error: int function must return a value")
                    self.analyze_expr(expr, vars_init)
                return True
            case _:
                raise CompileError(
                    f"semantic error: unsupported stmt node {type(n).__name__}"
                )

    def analyze_expr(self, n: Node, vars_init: Dict[str, bool]) -> str:
        match n:
            case IntLit():
                return "int"
            case Var(name=name):
                if name not in vars_init:
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                if not vars_init[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
                return "int"
            case BinOp(lhs=lhs, rhs=rhs):
                lt = self.analyze_expr(lhs, vars_init)
                rt = self.analyze_expr(rhs, vars_init)
                if lt != "int" or rt != "int":
                    raise CompileError("semantic error: binary op currently supports int only")
                return "int"
            case Call(name=name, args=args):
                sig = self.func_sigs.get(name)
                if sig is None:
                    raise CompileError(f"semantic error: call to undeclared function {name!r}")
                if len(args) != len(sig.param_types):
                    raise CompileError(
                        f"semantic error: function {name!r} expects {len(sig.param_types)} args, got {len(args)}"
                    )
                for i, arg in enumerate(args):
                    at = self.analyze_expr(arg, vars_init)
                    if at != sig.param_types[i]:
                        raise CompileError(
                            f"semantic error: argument type mismatch for {name!r} at index {i}"
                        )
                return sig.ret_type
            case _:
                raise CompileError(
                    f"semantic error: unsupported expr node {type(n).__name__}"
                )


class IRBuilder:
    def __init__(self, prog: Program):
        self.prog = prog
        self.tmp_idx = 0
        self.lines: List[str] = []
        self.slots: Dict[str, str] = {}
        self.func_sigs: Dict[str, FunctionSig] = {}
        for d in prog.decls:
            self.func_sigs[d.name] = FunctionSig(d.ret_type, [p.typ for p in d.params])
        for fn in prog.funcs:
            self.func_sigs[fn.name] = FunctionSig(fn.ret_type, [p.typ for p in fn.params])

    def llvm_ty(self, ty: str) -> str:
        return "void" if ty == "void" else "i32"

    def tmp(self) -> str:
        t = f"%t{self.tmp_idx}"
        self.tmp_idx += 1
        return t

    def emit(self, s: str) -> None:
        self.lines.append(s)

    def codegen_expr(self, n: Node) -> Optional[str]:
        match n:
            case IntLit(value=value):
                return str(value)
            case Var(name=name):
                slot = self.slots[name]
                t = self.tmp()
                self.emit(f"  {t} = load i32, ptr {slot}")
                return t
            case BinOp(op=op, lhs=lhs, rhs=rhs):
                l = self.codegen_expr(lhs)
                r = self.codegen_expr(rhs)
                if l is None or r is None:
                    raise CompileError("codegen error: void value used in binary operation")
                t = self.tmp()
                op_map = {"+": "add", "-": "sub", "*": "mul", "/": "sdiv"}
                self.emit(f"  {t} = {op_map[op]} i32 {l}, {r}")
                return t
            case Call(name=name, args=args):
                sig = self.func_sigs[name]
                args_text: List[str] = []
                for arg in args:
                    v = self.codegen_expr(arg)
                    if v is None:
                        raise CompileError("codegen error: void argument is not allowed")
                    args_text.append(f"i32 {v}")
                llvm_ret_ty = self.llvm_ty(sig.ret_type)
                call_text = f"call {llvm_ret_ty} @{name}({', '.join(args_text)})"
                if sig.ret_type == "void":
                    self.emit(f"  {call_text}")
                    return None
                t = self.tmp()
                self.emit(f"  {t} = {call_text}")
                return t
            case _:
                raise CompileError(f"codegen error: unsupported expr {type(n).__name__}")

    def codegen_stmt(self, n: Node, fn_ret_type: str) -> bool:
        match n:
            case Decl(name=name, init=init):
                slot = f"%{name}"
                self.slots[name] = slot
                self.emit(f"  {slot} = alloca i32")
                if init is not None:
                    v = self.codegen_expr(init)
                    if v is None:
                        raise CompileError("codegen error: cannot initialize int with void")
                    self.emit(f"  store i32 {v}, ptr {slot}")
                return False
            case Assign(name=name, expr=expr):
                v = self.codegen_expr(expr)
                if v is None:
                    raise CompileError("codegen error: cannot assign void to int")
                self.emit(f"  store i32 {v}, ptr {self.slots[name]}")
                return False
            case ExprStmt(expr=expr):
                self.codegen_expr(expr)
                return False
            case Return(expr=expr):
                if fn_ret_type == "void":
                    self.emit("  ret void")
                else:
                    if expr is None:
                        raise CompileError("codegen error: int function must return value")
                    v = self.codegen_expr(expr)
                    if v is None:
                        raise CompileError("codegen error: int function cannot return void")
                    self.emit(f"  ret i32 {v}")
                return True
            case _:
                raise CompileError(f"codegen error: unsupported stmt {type(n).__name__}")

    def emit_declarations(self) -> None:
        defined = {fn.name for fn in self.prog.funcs}
        seen: Dict[str, bool] = {}
        for d in self.prog.decls:
            if d.name in defined or d.name in seen:
                continue
            seen[d.name] = True
            ret_ty = self.llvm_ty(d.ret_type)
            params = ", ".join(self.llvm_ty(p.typ) for p in d.params)
            self.emit(f"declare {ret_ty} @{d.name}({params})")

    def codegen_function(self, fn: Function) -> None:
        self.tmp_idx = 0
        self.slots = {}

        ret_ty = self.llvm_ty(fn.ret_type)
        params_sig: List[str] = []
        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            params_sig.append(f"i32 %{pname}.arg")
        self.emit(f"define {ret_ty} @{fn.name}({', '.join(params_sig)}) {{")
        self.emit("entry:")

        for i, p in enumerate(fn.params):
            pname = p.name if p.name is not None else f"arg{i}"
            slot = f"%{pname}"
            self.slots[pname] = slot
            self.emit(f"  {slot} = alloca i32")
            self.emit(f"  store i32 %{pname}.arg, ptr {slot}")

        saw_return = False
        for st in fn.body:
            saw_return = self.codegen_stmt(st, fn.ret_type)
            if saw_return:
                break

        if not saw_return:
            if fn.ret_type == "void":
                self.emit("  ret void")
            else:
                self.emit("  ret i32 0")

        self.emit("}")

    def codegen_program(self) -> str:
        self.emit_declarations()
        if self.lines:
            self.emit("")
        for idx, fn in enumerate(self.prog.funcs):
            self.codegen_function(fn)
            if idx != len(self.prog.funcs) - 1:
                self.emit("")
        return "\n".join(self.lines) + "\n"


def compile_source(src: str) -> str:
    tokens = Lexer(src).scan()
    ast = Parser(tokens).parse_program()
    SemanticAnalyzer().analyze_program(ast)
    return IRBuilder(ast).codegen_program()


def main() -> int:
    ap = argparse.ArgumentParser(description="Tiny C-subset to LLVM IR compiler")
    ap.add_argument("input", help="Path to input .c file")
    ap.add_argument("-o", "--output", help="Path to output .ll file")
    args = ap.parse_args()

    with open(args.input, "r", encoding="utf-8") as f:
        src = f.read()

    try:
        ir = compile_source(src)
    except CompileError as e:
        print(f"compile error: {e}", file=sys.stderr)
        return 1

    if args.output:
        with open(args.output, "w", encoding="utf-8") as f:
            f.write(ir)
    else:
        sys.stdout.write(ir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
