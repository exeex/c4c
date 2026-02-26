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
    KW_RETURN = auto()
    ID = auto()
    NUM = auto()
    LBRACE = auto()
    RBRACE = auto()
    LPAREN = auto()
    RPAREN = auto()
    SEMI = auto()
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
        if text == "int":
            typ = TokenType.KW_INT
        elif text == "return":
            typ = TokenType.KW_RETURN
        else:
            typ = TokenType.ID
        return Token(typ, text, start_line, start_col)

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
class Program(Node):
    fn: "Function"


@dataclass
class Function(Node):
    name: str
    body: List[Node]


@dataclass
class Decl(Node):
    name: str
    init: Optional[Node]


@dataclass
class Assign(Node):
    name: str
    expr: Node


@dataclass
class Return(Node):
    expr: Node


@dataclass
class IntLit(Node):
    value: int


@dataclass
class Var(Node):
    name: str


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

    def parse_program(self) -> Program:
        fn = self.parse_function()
        self.eat(TokenType.EOF)
        return Program(fn)

    def parse_function(self) -> Function:
        self.eat(TokenType.KW_INT)
        name = self.eat(TokenType.ID).text
        self.eat(TokenType.LPAREN)
        self.eat(TokenType.RPAREN)
        self.eat(TokenType.LBRACE)
        body: List[Node] = []
        while self.cur().typ != TokenType.RBRACE:
            body.append(self.parse_stmt())
        self.eat(TokenType.RBRACE)
        return Function(name, body)

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
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Return(expr)
            case TokenType.ID:
                name = self.eat(TokenType.ID).text
                self.eat(TokenType.ASSIGN)
                expr = self.parse_expr()
                self.eat(TokenType.SEMI)
                return Assign(name, expr)
            case _:
                raise self.error("unknown statement")

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

    def parse_factor(self) -> Node:
        match self.cur().typ:
            case TokenType.NUM:
                return IntLit(int(self.eat(TokenType.NUM).text))
            case TokenType.ID:
                return Var(self.eat(TokenType.ID).text)
            case TokenType.LPAREN:
                self.advance()
                n = self.parse_expr()
                self.eat(TokenType.RPAREN)
                return n
            case _:
                raise self.error("expected expression")


class SemanticAnalyzer:
    def __init__(self):
        self.vars: Dict[str, bool] = {}
        self.saw_return = False

    def analyze_program(self, p: Program) -> None:
        self.analyze_function(p.fn)

    def analyze_function(self, fn: Function) -> None:
        for st in fn.body:
            self.analyze_stmt(st)
            if self.saw_return:
                break

    def analyze_stmt(self, n: Node) -> None:
        match n:
            case Decl(name=name, init=init):
                if name in self.vars:
                    raise CompileError(f"semantic error: variable redeclared: {name!r}")
                self.vars[name] = False
                if init is not None:
                    self.analyze_expr(init)
                    self.vars[name] = True
            case Assign(name=name, expr=expr):
                if name not in self.vars:
                    raise CompileError(
                        f"semantic error: assignment to undeclared variable {name!r}"
                    )
                self.analyze_expr(expr)
                self.vars[name] = True
            case Return(expr=expr):
                self.analyze_expr(expr)
                self.saw_return = True
            case _:
                raise CompileError(
                    f"semantic error: unsupported stmt node {type(n).__name__}"
                )

    def analyze_expr(self, n: Node) -> None:
        match n:
            case IntLit():
                return
            case Var(name=name):
                if name not in self.vars:
                    raise CompileError(
                        f"semantic error: use of undeclared variable {name!r}"
                    )
                if not self.vars[name]:
                    raise CompileError(
                        f"semantic error: use of uninitialized variable {name!r}"
                    )
            case BinOp(lhs=lhs, rhs=rhs):
                self.analyze_expr(lhs)
                self.analyze_expr(rhs)
            case _:
                raise CompileError(
                    f"semantic error: unsupported expr node {type(n).__name__}"
                )


class IRBuilder:
    def __init__(self):
        self.tmp_idx = 0
        self.lines: List[str] = []
        self.slots: Dict[str, str] = {}

    def tmp(self) -> str:
        t = f"%t{self.tmp_idx}"
        self.tmp_idx += 1
        return t

    def emit(self, s: str) -> None:
        self.lines.append(s)

    def codegen_expr(self, n: Node) -> str:
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
                t = self.tmp()
                op_map = {"+": "add", "-": "sub", "*": "mul", "/": "sdiv"}
                self.emit(f"  {t} = {op_map[op]} i32 {l}, {r}")
                return t
            case _:
                raise CompileError(f"codegen error: unsupported expr {type(n).__name__}")

    def codegen_stmt(self, n: Node) -> bool:
        match n:
            case Decl(name=name, init=init):
                slot = f"%{name}"
                self.slots[name] = slot
                self.emit(f"  {slot} = alloca i32")
                if init is not None:
                    v = self.codegen_expr(init)
                    self.emit(f"  store i32 {v}, ptr {slot}")
                return False
            case Assign(name=name, expr=expr):
                v = self.codegen_expr(expr)
                self.emit(f"  store i32 {v}, ptr {self.slots[name]}")
                return False
            case Return(expr=expr):
                v = self.codegen_expr(expr)
                self.emit(f"  ret i32 {v}")
                return True
            case _:
                raise CompileError(f"codegen error: unsupported stmt {type(n).__name__}")

    def codegen_program(self, p: Program) -> str:
        fn = p.fn
        self.emit(f"define i32 @{fn.name}() {{")
        self.emit("entry:")
        saw_return = False
        for st in fn.body:
            saw_return = self.codegen_stmt(st)
            if saw_return:
                break
        if not saw_return:
            self.emit("  ret i32 0")
        self.emit("}")
        return "\n".join(self.lines) + "\n"


def compile_source(src: str) -> str:
    tokens = Lexer(src).scan()
    ast = Parser(tokens).parse_program()
    SemanticAnalyzer().analyze_program(ast)
    return IRBuilder().codegen_program(ast)


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
