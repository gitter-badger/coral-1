%language "c++"
%define api.value.type variant
%lex-param {pp}
%name-prefix "coral"
%parse-param {ParserParam pp}

%{
#include "lexer-internal.hh"
#include <cstdio>
%}

%token <std::string> OP OP4 OP3 OP2 OP1
%token <std::string> OP_EQ
%token <std::string> COMMENT
%token <std::string> STRING
%token <std::string> IDENTIFIER
%token <std::string> INTEGER
%token NEWLINE
%token INDENT
%token DEDENT
%token FUNC
%token LET
%token FOR
%token IN
%token IF
%token ELIF
%token ELSE
%token RETURN
%token ENDOFFILE 0

// TODO split between Statements and Exprs
%type <coral::ast::BaseExpr *> Expr Atom Expr2 Expr3 Expr4
%type <coral::ast::BaseExpr *> Function ModuleLine ModuleRule ForLoop Let Argument
%type <coral::ast::IfExpr *> IfStatement
%type <coral::ast::Block *> StatementBlock
%type <coral::ast::TupleLiteral *> Tuple
%type <std::vector<coral::ast::BaseExpr *>> BlockLines ArgumentsListInner
%type <std::vector<std::string>> IdentifierList
%type <coral::type::Type *> TypeDef

%type <coral::ast::Def *> Param
%type <std::vector<coral::ast::Def *>> ParamsListInner

%%

ModuleRule : BlockLines ENDOFFILE {
  pp->module = new coral::ast::Module($1);
  return 0;
}

BlockLines : { }
| BlockLines ModuleLine NEWLINE { $$ = $1; $$.push_back($2); }
| BlockLines NEWLINE { $$ = $1; }

StatementBlock
: ':' NEWLINE INDENT BlockLines DEDENT { $$ = new ast::Block($4); }
| ':' ModuleLine { $$ = new ast::Block({ $2 }); }

Atom // Expr0 - Can be called without parens
: IDENTIFIER { $$ = new ast::Var($1); }
| INTEGER { $$ = new ast::IntLiteral($1); }
| STRING { $$ = new ast::StringLiteral($1); }
| '[' ArgumentsListInner ']' { $$ = new ast::ListLiteral($2); }
| '(' Expr ')' { $$ = $2; }

Tuple : '(' ArgumentsListInner ')' { $$ = new ast::TupleLiteral($2); }

Expr2 : Atom { $$ = $1; } // Expr2s can be unparenthesized in binary ops
| Expr2 '.' IDENTIFIER { $$ = new ast::Member($1, $3); }
| Expr2 Tuple { $$ = new ast::Call($1, $2); delete $2; }
| Expr2 '(' ')' { $$ = new ast::Call($1, std::vector<ast::BaseExpr *>()); }
| Expr2 Atom { $$ = new ast::Call($1, { $2 }); }
Expr3 : Expr2 { $$ = $1; } | Tuple { $$ = $1; } | Expr3 OP2 Expr3 { $$ = new ast::BinOp($1, $2, $3); }
Expr4 : Expr3 { $$ = $1; } | Expr4 OP3 Expr4 { $$ = new ast::BinOp($1, $2, $3); }
Expr : Expr4 { $$ = $1; }
| Expr4 OP4 Expr4  { $$ = new ast::BinOp($1, $2, $3); }
| Expr4 OP_EQ Expr4  { $$ = new ast::BinOp($1, $2, $3); }
| Expr4 OP Expr4 { $$ = new ast::BinOp($1, $2, $3); }

ArgumentsListInner : Argument { $$.push_back($1); }
| ArgumentsListInner ',' Argument { $$ = $1; $$.push_back($3); }

Argument : Expr { $$ = $1; } | IDENTIFIER '=' Expr { $$ = $3; }

Param : IDENTIFIER { $$ = new ast::Def($1, 0, 0); }
| IDENTIFIER ':' TypeDef { $$ = new ast::Def($1, $3, 0); }

TypeDef : IDENTIFIER { $$ = new coral::type::Type($1); }

ParamsListInner : Param { $$.push_back($1); }
| ParamsListInner ',' Param { $$ = $1; $$.push_back($3); }

ModuleLine
: COMMENT { $$ = new ast::Comment($1); }
| Expr { $$ = $1; }
| Function { $$ = $1; }
| Let { $$ = $1; }
| ForLoop { $$ = $1; }
| IfStatement { $$ = $1; }
| RETURN Expr { $$ = new ast::Return($2); }

Function
: FUNC IDENTIFIER '(' ParamsListInner ')' StatementBlock { $$ = new ast::Func($2, new Type("o"), $4, $6); }
| FUNC IDENTIFIER ':' TypeDef '(' ParamsListInner ')' StatementBlock {
   $$ = new ast::Func($2, $4, $6, $8); }
| FUNC IDENTIFIER '(' ')' StatementBlock { $$ = new ast::Func($2, new Type("o"), {}, $5); }
| '`' IDENTIFIER FUNC IDENTIFIER ':' TypeDef '(' ParamsListInner ')' {
  $$ = new ast::Func($4, $6, $8, 0); }
ForLoop
: FOR IDENTIFIER IN Expr StatementBlock { $$ = new ast::ForExpr(new ast::Var($2), $4, $5); }
| FOR IdentifierList IN Expr StatementBlock { $$ = new ast::ForExpr(new ast::Var($2), $4, $5); }

IfStatement
: IF Expr StatementBlock { $$ = new ast::IfExpr($2, $3, 0); }
| IF Expr StatementBlock ELSE StatementBlock %prec ELSE { $$ = new ast::IfExpr($2, $3, $5); }
| IF Expr StatementBlock ELSE IfStatement %prec ELSE { $$ = new ast::IfExpr($2, $3, new ast::Block({ $5 })); }

Let : LET IDENTIFIER OP_EQ Expr { $$ = new ast::Let(new ast::Var($2), $4); }

IdentifierList : IDENTIFIER { $$.push_back($1); }
| IdentifierList ',' IDENTIFIER { $$ = $1; $$.push_back($3); }

%%