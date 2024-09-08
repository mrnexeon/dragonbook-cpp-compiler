#pragma once
#include "Lexer.h"
#include "Symbols.h"
#include "Inter.h"

/*
	Symbol table
*/
class Env {
public:
	Env(std::shared_ptr<Env> n) : prev(n) {}
	std::shared_ptr<Env> prev;
	void put(std::shared_ptr<Token> w, std::shared_ptr<Id> i) {
		table.emplace(w, i);
	}
	std::shared_ptr<Id> get(std::shared_ptr<Token> w) {
		for (Env* e = this; e != nullptr; e = e->prev.get()) {
			auto result = e->table.find(w);
			if (result != e->table.end()) return result->second;
		}
		return nullptr;
	}
private:
	std::map<std::shared_ptr<Token>, std::shared_ptr<Id> > table;
};

class Parser {
public:
	std::shared_ptr<Env> top; // Current or top symbol table
	int used;
	Parser(std::shared_ptr<Lexer> lexer) : lex(lexer), used(0) { 
		lexer->reserve(std::make_shared<Word>("if", IF));
		lexer->reserve(std::make_shared<Word>("else", ELSE));
		lexer->reserve(std::make_shared<Word>("while", WHILE));
		lexer->reserve(std::make_shared<Word>("do", DO));
		lexer->reserve(std::make_shared<Word>("break", BREAK));
		lexer->reserve(Word::True);
		lexer->reserve(Word::False);
		lexer->reserve(Type::Int);
		lexer->reserve(Type::Float);
		lexer->reserve(Type::Char);
		lexer->reserve(Type::Bool);
		move(); 
	}
	void move() {
		look = lex->scan();
	}
	void error(std::string s) {
		std::stringstream ss;
		ss << "near line " << lex->line << ": " << s;
		throw std::runtime_error(ss.str());
	}
	void match(int t) {
		if (look->tag == t) move();
		else error("syntax error");
	}

	// program -> block
	std::shared_ptr<Stmt> program() {
		// It starts to produce AST
		std::shared_ptr<Stmt> s = block();
	
		// It generates the beginning of the program
		int begin = s->newlabel();
		int after = s->newlabel();
		s->emitlabel(begin);
		s->gen(begin, after);
		s->emitlabel(after);

		return s;
	}

	// block -> decls stmts
	std::shared_ptr<Stmt> block() {
		match('{'); std::shared_ptr<Env> savedEnv = top;
		top = std::make_shared<Env>(top);
		decls(); std::shared_ptr<Stmt> s = stmts();
		match('}'); top = savedEnv;
		return s;
	}

	void decls() {
		while (look->tag == BASIC) {
			// D -> Type Id
			std::shared_ptr<Type> p = type(); std::shared_ptr<Token> tok = look;
			match(ID); match(';');
			std::shared_ptr<Id> id = std::make_shared<Id>(std::dynamic_pointer_cast<Word>(tok), p, used);
			top->put(tok, id);
			used += p->width;
		}
	}

	std::shared_ptr<Type> type() {
		std::shared_ptr<Type> p = std::dynamic_pointer_cast<Type>(look);
		match(BASIC); 
		if (look->tag != '[') return p;
		else return dims(p);
	}

	std::shared_ptr<Type> dims(std::shared_ptr<Type> p) {
		match('['); std::shared_ptr<Token> tok = look;
		match(NUM); match(']');
		if (look->tag == '[') p = dims(p);
		return std::make_shared<Array>((std::dynamic_pointer_cast<Num>(tok))->value, p);
	}

	std::shared_ptr<Stmt> stmts() {
		if (look->tag == '}') {
			return Stmt::Null;
		}
		else {
			auto s = stmt(); auto ss = stmts();
			return std::make_shared<Seq>(s, ss);
		}
	}

	std::shared_ptr<Stmt> stmt() {
		std::shared_ptr<Expr> x; std::shared_ptr<Stmt> s, s1, s2;
		std::shared_ptr<Stmt> savedStmt; // Save enclosing statement for break

		switch (look->tag) {
		case ';':
			move();
			return Stmt::Null;
			break;
		case IF:
			match(IF); match('(');
			x = boolean(); match(')');
			s1 = stmt();
			if (look->tag != ELSE) {
				return std::make_shared<If>(x, s1);
			}
			match(ELSE);
			s2 = stmt();
			return std::make_shared<Else>(x, s1, s2);
			break;
		case WHILE:
			{
				std::shared_ptr<While> w = std::make_shared<While>();
				savedStmt = Stmt::Enclosing;
				Stmt::Enclosing = w;
				match(WHILE); match('(');
				x = boolean(); match(')');
				s1 = stmt();
				w->Init(x, s1);
				Stmt::Enclosing = savedStmt;
				return w;
			}
			break;
		case DO:
			{
				std::shared_ptr<Do> d = std::make_shared<Do>();
				savedStmt = Stmt::Enclosing;
				Stmt::Enclosing = d;
				match(DO);
				s1 = stmt();
				match(WHILE); match('(');
				x = boolean(); match(')');
				d->Init(s1, x);
				Stmt::Enclosing = savedStmt;
				return d;
			}
			break;
		case BREAK:
			match(BREAK); match(';');
			return std::make_shared<Break>();
			break;
		case '{':
			return block();
			break;
		default:
			return assign();
			break;
		}
	}

	std::shared_ptr<Stmt> assign() {
		std::shared_ptr<Stmt> stmt; std::shared_ptr<Token> tok = look;
		match(ID);
		std::shared_ptr<Id> id = top->get(tok);
		if (id == nullptr) error(tok->toString() + " undeclared");
		if (look->tag == '=') {
			move(); stmt = std::make_shared<Set>(id, boolean());
		}
		else {
			std::shared_ptr<Access> x = offset(id);
			match('='); stmt = std::make_shared<SetElem>(x, boolean());
		}
		match(';');
		return stmt;
	}

	std::shared_ptr<Expr> boolean() {
		std::shared_ptr<Expr> x = join();
		while (look->tag == OR) {
			std::shared_ptr<Token> tok = look; move();
			x = std::make_shared<Or>(tok, x, join());
		}
		return x;
	}

	std::shared_ptr<Expr> join() {
		std::shared_ptr<Expr> x = equality();
		while (look->tag == AND) {
			std::shared_ptr<Token> tok = look; move();
			x = std::make_shared<And>(tok, x, equality());
		}
		return x;
	}

	std::shared_ptr<Expr> equality() {
		std::shared_ptr<Expr> x = rel();
		while (look->tag == EQ) {
			std::shared_ptr<Token> tok = look; move();
			x = std::make_shared<Rel>(tok, x, rel());
		}
		return x;
	}

	std::shared_ptr<Expr> rel() {
		std::shared_ptr<Expr> x = expr();
		switch (look->tag) {
		case '<':
		case LE:
		case GE:
		case '>':
			{
				std::shared_ptr<Token> tok = look; move();
				x = std::make_shared<Rel>(tok, x, expr());
			}
		default:
			return x;
			break;
		}
	}

	std::shared_ptr<Expr> expr() {
		std::shared_ptr<Expr> x = term();
		while (look->tag == '+' || look->tag == '-') {
			std::shared_ptr<Token> tok = look; move();
			x = std::make_shared<Arith>(tok, x, term());
		}
		return x;
	}

	std::shared_ptr<Expr> term() {
		std::shared_ptr<Expr> x = unary();
		while (look->tag == '*' || look->tag == '/') {
			std::shared_ptr<Token> tok = look; move();
			x = std::make_shared<Arith>(tok, x, unary());
		}
		return x;
	}

	std::shared_ptr<Expr> unary() {
		if (look->tag == '-') {
			move();
			return std::make_shared<Unary>(Word::Minus, unary());
		}
		else if (look->tag == '!') {
			std::shared_ptr<Token> tok = look; move();
			return std::make_shared<Not>(tok, unary());
		}
		else return factor();
	}

	std::shared_ptr<Expr> factor() {
		std::shared_ptr<Expr> x;
		switch (look->tag) {
		case '(':
			move(); x = boolean(); match(')');
			return x;
			break;
		case NUM:
			x = std::make_shared<Constant>(look, Type::Int);
			move(); return x;
			break;
		case REAL:
			x = std::make_shared<Constant>(look, Type::Float);
			move(); return x;
			break;
		case TRUE:
			x = Constant::True;
			move(); return x;
			break;
		case FALSE:
			x = Constant::False;
			move(); return x;
			break;
		case ID:
			{
				std::string s = look->toString();
				std::shared_ptr<Id> id = top->get(look);
				if (id == nullptr) {
					error(look->toString() + " undeclared");
				}
				move();
				if (look->tag != '[') return id;
				else return offset(id);
			}
			break;
		default:
			error("syntax error");
			return nullptr;
			break;
		}
		
		return nullptr;
	}

	std::shared_ptr<Access> offset(std::shared_ptr<Id> a) {
		std::shared_ptr<Expr> i, w, t1, t2, loc;
		std::shared_ptr<Type> type = a->type;
		match('['); i = boolean(); match(']');
		type = (std::dynamic_pointer_cast<Array>(type))->of;
		w = std::make_shared<Constant>(type->width);
		t1 = std::make_shared<Arith>(std::make_shared<Token>('*'), i, w);
		loc = t1;
		while (look->tag == '[') {
			match('['); i = boolean(); match(']');
			type = (std::dynamic_pointer_cast<Array>(type))->of;
			w = std::make_shared<Constant>(type->width);
			t1 = std::make_shared<Arith>(std::make_shared<Token>('*'), i, w);
			t2 = std::make_shared<Arith>(std::make_shared<Token>('+'), loc, t1);
			loc = t2;
		}
		return std::make_shared<Access>(a, loc, type);
	}
private:
	std::shared_ptr<Lexer> lex;
	std::shared_ptr<Token> look;
};