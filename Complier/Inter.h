#pragma once
#include "Lexer.h"
#include "Symbols.h"
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

/*
	Узел синтаксического дерева
*/
class Node {
public:
	Node() : lexline(Lexer::line) {}

	int lexline;
	void error(std::string s) {
		throw std::exception(s.c_str());
	}

	virtual json toJson() {
		return json({ {"name", "Node" } });
	}

	virtual uint32_t toDot(std::stringstream &ss) {
		uint32_t id = hash(this);
		ss << '\t' << id << ' ' << "[shape=box, label=\"Node\"]" << std::endl;
		return id;
	}

	// Для генерации промежуточного трехадресного кода
	static int labels;
	static std::hash<Node*> hash;

	int newlabel() { return ++labels; }
	void emitlabel(int i) { std::cout << "L" << i << ":"; }
	void emit(std::string s) { std::cout << "\t" << s << std::endl; }
};

int Node::labels = 0;
std::hash<Node*> Node::hash = std::hash<Node*>();

/*
	Узел для выражений
*/
class Expr : public Node, public std::enable_shared_from_this<Expr> {
public:
	Expr(std::shared_ptr<Token> t, std::shared_ptr<Type> p) : op(t), type(p) {}

	std::shared_ptr<Token> op;
	std::shared_ptr<Type> type;

	virtual std::shared_ptr<Expr> gen() { return shared_from_this();  }
	virtual std::shared_ptr<Expr> reduce() { return shared_from_this(); }
	virtual void jumping(int t, int f) {
		emitjumps(toString(), t, f);
	}
	virtual void emitjumps(std::string test, int t, int f) {
		std::stringstream ss;

		if (t != 0 && f != 0) {
			ss << "if " << test << " goto L" << t;
			emit(ss.str());
			ss.clear();
			ss << "goto L" << f;
			emit(ss.str());
		}
		else if (t != 0) {
			ss << "if " << test << " goto L" << t;
			emit(ss.str());
		}
		else if (f != 0) {
			ss << "iffalse " << test << " goto L" << f;
			emit(ss.str());
		}
	}

	virtual std::string toString() { return op->toString(); }
};

/*
	Узел временных переменных для трехадресного кода
	(Не включен в АСД)
*/
class Temp : public Expr {
public:
	Temp(std::shared_ptr<Type> p) : Expr(Word::Temp, p), number(++count) {}

	static int count;
	int number;
	std::string toString() override { 
		std::stringstream ss;
		ss << 't' << number;
		return ss.str(); 
	}
};

int Temp::count = 0;

/*
	Узел абстрактного оператора с двумя операндами
*/
class Op : public Expr {
public:
	Op(std::shared_ptr<Token> t, std::shared_ptr<Type> p) : Expr(t, p) {}
	std::shared_ptr<Expr> reduce() override {
		std::shared_ptr<Expr> x = gen();
		std::shared_ptr<Temp> t = std::make_shared<Temp>(type);

		emit(t->toString() + " = " + x->toString());
		return t;
	}
};

/*
	Узел арифметической операции
*/
class Arith : public Op {
public:
	std::shared_ptr<Expr> expr1, expr2;
	Arith(std::shared_ptr<Token> t, std::shared_ptr<Expr> x1, std::shared_ptr<Expr> x2) : Op(t, nullptr), expr1(x1), expr2(x2) {
		type = Type::max(expr1->type, expr2->type);
		if (type == nullptr) error("type error");
	}

	json toJson() override {
		json a = expr1->toJson();
		json b = expr2->toJson();

		json j = { { "name", "Arith" }, {"op", op->toString()}, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Arith\\nop: " << op->toString() << "\"" << ", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr1->toDot(ss) << std::endl;;
		ss << '\t' << i << " -> " << expr2->toDot(ss) << std::endl;;

		return i;
	}

	std::shared_ptr<Expr> gen() override {
		auto x1 = expr1->reduce();
		auto x2 = expr2->reduce();
		return std::make_shared<Arith>(op, x1, x2);
	}

	std::string toString() override {
		return expr1->toString() + " " + op->toString() + " " + expr2->toString();
	}
};

/*
	Узел унарного оператора
*/
class Unary : public Op {
public:
	std::shared_ptr<Expr> expr;
	Unary(std::shared_ptr<Token> t, std::shared_ptr<Expr> x) : Op(t, nullptr), expr(x) {
		type = Type::max(Type::Int, x->type);
		if (type == nullptr) error("type error");
	}

	json toJson() override {
		json a = expr->toJson();

		json j = { { "name", "Unary" }, {"children", { a }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Unary\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;;

		return i;
	}

	std::shared_ptr<Expr> gen() override {
		return std::make_shared<Unary>(op, expr->reduce());
	}

	std::string toString() override {
		return op->toString() + " " + expr->toString();
	}
};

/*
	Узел константы
*/
class Constant : public Expr {
public:
	Constant(std::shared_ptr<Token> t, std::shared_ptr<Type> p) : Expr(t, p) {}
	Constant(int i) : Expr(std::make_shared<Num>(i), Type::Int) {}
	
	static std::shared_ptr<Constant> True;
	static std::shared_ptr<Constant> False;

	json toJson() override {
		json j = { { "name", "Constant" }, {"val", op->toString()} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t id = hash(this);
		ss << '\t' << id << ' ' << "[shape=box, label=\"Const\\nval: " << op->toString() << "\"" << ", fillcolor=\"#f1f8e9\", style=filled]" << std::endl;
		return id;
	}

	void jumping(int t, int f) override {
		std::stringstream ss;
		if (this == True.get() && t != 0) {
			ss << "goto L" << t;
			emit(ss.str());
		}
		else if (this == False.get() && f != 0) {
			ss << "goto L" << f;
			emit(ss.str());
		}
	}


};

std::shared_ptr<Constant> Constant::True  = std::make_shared<Constant>(Word::True, Type::Bool);
std::shared_ptr<Constant> Constant::False = std::make_shared<Constant>(Word::False, Type::Bool);

/*
	Узел идентификатора
*/
class Id : public Expr {
public:
	Id(std::shared_ptr<Word> id, std::shared_ptr<Type> p, int b) : Expr(id, p), offset(b) {}
	int offset;

	json toJson() override {
		json j = { { "name", "Id" }, {"var", op->toString()} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		Id* temp = new Id(*this);
		uint32_t id = hash(temp) + ss.str().size();
		delete temp;
		ss << '\t' << id << ' ' << "[shape=box, label=\"Id\\nvar: " << op->toString() << "\"" << ", fillcolor=\"#f1f8e9\", style=filled]" << std::endl;
		return id;
	}
};

/*
	Узел логической операции
*/
class Logical : public Expr {
public:
	std::shared_ptr<Expr> expr1, expr2;
	Logical(std::shared_ptr<Token> t, std::shared_ptr<Expr> x1, std::shared_ptr<Expr> x2) : Expr(t, nullptr), expr1(x1), expr2(x2) {

	}

	virtual std::shared_ptr<Type> check(std::shared_ptr<Type> p1, std::shared_ptr<Type> p2) {
		if (p1 == Type::Bool && p2 == Type::Bool) return Type::Bool;
		else {
			error("type error");
			return nullptr;
		}
	}

	json toJson() override {
		json a = expr1->toJson();
		json b = expr2->toJson();

		json j = { { "name", "Logical" }, {"op", op->toString()},  {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Logical\\nop: " << op->toString() << "\"" << ", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr1->toDot(ss) << std::endl;;
		ss << '\t' << i << " -> " << expr2->toDot(ss) << std::endl;;

		return i;
	}

	std::shared_ptr<Expr> gen() override {
		std::stringstream ss;
		int f = newlabel(); int a = newlabel();
		std::shared_ptr<Temp> t = std::make_shared<Temp>(type);
		this->jumping(0, f);
		ss << t->toString() << " = true";
		emit(ss.str()); ss.clear();
		ss << "goto L" << a;
		emit(ss.str()); ss.clear();
		emitlabel(f);
		ss << t->toString() << " = false";
		emit(ss.str()); ss.clear();
		emitlabel(a);
		return t;
	}

	std::string toString() override {
		return expr1->toString() + " " + op->toString() + " " + expr2->toString();
	}
};

/*
	Узел операции ИЛИ
*/
class Or : public Logical {
public:
	Or(std::shared_ptr<Token> t, std::shared_ptr<Expr> x1, std::shared_ptr<Expr> x2) : Logical(t, x1, x2) {
		type = check(x1->type, x2->type);
	}
	void jumping(int t, int f) override {
		int label = t != 0 ? t : newlabel();
		expr1->jumping(label, 0);
		expr2->jumping(t, f);
		if (t == 0) emitlabel(label);

	}
};

/*
	Узел операции И
*/
class And : public Logical {
public:
	And(std::shared_ptr<Token> t, std::shared_ptr<Expr> x1, std::shared_ptr<Expr> x2) : Logical(t, x1, x2) {
		type = check(x1->type, x2->type);
	}
	void jumping(int t, int f) override {
		int label = f != 0 ? f : newlabel();
		expr1->jumping(label, 0);
		expr2->jumping(t, f);
		if (f == 0) emitlabel(label);

	}
};

/*
	Узел операции НЕ
*/
class Not : public Logical {
public:
	Not(std::shared_ptr<Token> t,  std::shared_ptr<Expr> x2) : Logical(t, x2, x2) {
		type = check(x2->type, x2->type);
	}
	void jumping(int t, int f) override {
		expr2->jumping(f, t);
	}
	std::string toString() override {
		return op->toString() + " " + expr2->toString();
	}
};

/*
	Узел отношения
*/
class Rel : public Logical {
public:
	Rel(std::shared_ptr<Token> t, std::shared_ptr<Expr> x1, std::shared_ptr<Expr> x2) : Logical(t, x1, x2) {
		type = check(x1->type, x2->type);
	}

	json toJson() override {
		json a = expr1->toJson();
		json b = expr2->toJson();

		json j = { { "name", "Rel" }, {"op", op->toString()}, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Rel\\nop: " << op->toString() << "\"" << ", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr1->toDot(ss) << std::endl;;
		ss << '\t' << i << " -> " << expr2->toDot(ss) << std::endl;;

		return i;
	}

	std::shared_ptr<Type> check(std::shared_ptr<Type> p1, std::shared_ptr<Type> p2) override {
		Array* arr_left = dynamic_cast<Array*>(p1.get());
		Array* arr_right = dynamic_cast<Array*>(p2.get());
		if (arr_left || arr_right) return nullptr;
		else if (p1 == p2) return Type::Bool;
		else return nullptr;
	}

	void jumping(int t, int f) override {
		std::shared_ptr<Expr> a = expr1->reduce();
		std::shared_ptr<Expr> b = expr2->reduce();
		std::string test = a->toString() + " " + op->toString() + " " + b->toString();
		emitjumps(test, t, f);
	}
};
/*
	Узел доступа к элементу массива
*/
class Access : public Op {
public:
	Access(std::shared_ptr<Id> a, std::shared_ptr<Expr> i, std::shared_ptr<Type> p) : Op(std::make_shared<Word>("[]", INDEX), p), arr(a), index(i) {}

	std::shared_ptr<Id> arr;
	std::shared_ptr<Expr> index;

	json toJson() override {
		json a = arr->toJson();
		//json b = index->toJson(); // infinite loop

		json j = { { "name", "Access" }, {"children", { a }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Access\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << arr->toDot(ss) << std::endl;
		//ss << '\t' << i << " -> " << index->toDot(ss) << std::endl; // infinite loop

		return i;
	}

	std::shared_ptr<Expr> gen() override {
		return std::make_shared<Access>(arr, index->reduce(), type);
	}

	void jumping(int t, int f) override {
		emitjumps(reduce()->toString(), t, f);
	}

	std::string toString() override {
		return arr->toString() + " [ " + index->toString() + " ]";
	}
};

/*
	Узел с инструкцией
*/
class Stmt : public Node {
public:
	Stmt() = default;
	static std::shared_ptr<Stmt> Null; // Пустая последовательность инструкций

	json toJson() override {
		if (this == Null.get()) return json({ {"name", "Empty" } });
		else return json({ {"name", "Stmt" } });
	}

	uint32_t toDot(std::stringstream& ss) override {
		if (this == Null.get()) {
			uint32_t id = hash(this) + ss.str().size();
			ss << '\t' << id << ' ' << "[shape=box, label=\"Empty\", fillcolor=\"#eceff1\", style=filled]" << std::endl;
			return id;
		}
		else {
			uint32_t id = hash(this);
			ss << '\t' << id << ' ' << "[shape=box, label=\"Stmt\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;
			return id;
		}
	}

	// Для генерации промежуточного трехадресного кода:
	static std::shared_ptr<Stmt> Enclosing;
	virtual void gen(int b, int a) {}
	int after;
};

std::shared_ptr<Stmt> Stmt::Null	  = std::make_shared<Stmt>();
std::shared_ptr<Stmt> Stmt::Enclosing = Null;

/*
	Узел блока If
*/
class If : public Stmt {
public:
	If(std::shared_ptr<Expr> x, std::shared_ptr<Stmt> s) : expr(x), stmt(s) {
		if (x->type != Type::Bool) x->error("Boolean required in If");
	}

	std::shared_ptr<Expr> expr;
	std::shared_ptr<Stmt> stmt;

	json toJson() override {
		json a = expr->toJson();
		json b = stmt->toJson();

		json j = { { "name", "If" }, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"If\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << stmt->toDot(ss) << std::endl;

		return i;
	}

	void gen(int b, int a) override {
		int label = newlabel();
		expr->jumping(0, a);
		emitlabel(label);
		stmt->gen(label, a);
	}
};

/*
	Узел блока If-Else
*/
class Else : public Stmt {
public:
	std::shared_ptr<Expr> expr;
	std::shared_ptr<Stmt> stmt1, stmt2;
	Else(std::shared_ptr<Expr> x, std::shared_ptr<Stmt> s1, std::shared_ptr<Stmt> s2) : expr(x), stmt1(s1), stmt2(s2) {
		if (x->type != Type::Bool) x->error("Boolean required in If-Else");
	}

	json toJson() override {
		json a = expr->toJson();
		json b = stmt1->toJson();
		auto c = stmt2->toJson();

		json j = { { "name", "If-Else" }, {"children", { a, b, c }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"If-Else\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << stmt1->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << stmt2->toDot(ss) << std::endl;


		return i;
	}

	void gen(int b, int a) override {
		std::stringstream ss;
		int label1 = newlabel();
		int label2 = newlabel();
		expr->jumping(0, label2);
		emitlabel(label1);
		stmt1->gen(label1, a);
		ss << "goto L" << a;
		emit(ss.str());
		emitlabel(label2);
		stmt2->gen(label2, a);
	}
};

/*
	Узел блока While
*/
class While : public Stmt {
public:
	While() = default;

	std::shared_ptr<Expr> expr;
	std::shared_ptr<Stmt> stmt;

	void Init(std::shared_ptr<Expr> x, std::shared_ptr<Stmt> s) {
		expr = x; stmt = s;
		if (x->type != Type::Bool) x->error("Boolean required in While");
	}

	json toJson() override {
		json a = expr->toJson();
		json b = stmt->toJson();

		json j = { { "name", "While" }, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"While\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << stmt->toDot(ss) << std::endl;

		return i;
	}

	void gen(int b, int a) override {
		std::stringstream ss;
		after = a;
		expr->jumping(0, a);
		int label = newlabel();
		emitlabel(label);
		stmt->gen(label, b);
		ss << "goto L" << b;
		emit(ss.str());
	}
};

/*
	Узел блока Do-While
*/
class Do : public Stmt {
public:
	Do() = default;

	std::shared_ptr<Expr> expr;
	std::shared_ptr<Stmt> stmt;

	void Init(std::shared_ptr<Stmt> s, std::shared_ptr<Expr> x) {
		expr = x; stmt = s;
		if (x->type != Type::Bool) x->error("Boolean required in Do-While");
	}

	json toJson() override {
		json a = expr->toJson();
		json b = stmt->toJson();

		json j = { { "name", "Do-While" }, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Do-While\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << stmt->toDot(ss) << std::endl;

		return i;
	}

	void gen(int b, int a) override {
		after = a;
		int label = newlabel();
		stmt->gen(b, label);
		emitlabel(label);
		expr->jumping(b, 0);
	}
};

/*
	Узел оператора присваивания 
*/
class Set : public Stmt {
public:
	Set(std::shared_ptr<Id> i, std::shared_ptr<Expr> x) : id(i), expr(x) {
		if (check(i->type, x->type) == nullptr) error("type error");
	}

	std::shared_ptr<Id> id;
	std::shared_ptr<Expr> expr;

	json toJson() override {
		json a = id->toJson();
		json b = expr->toJson();

		json j = { { "name", "Set" }, {"children", { a, b }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Assign\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << id->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;

		return i;
	}

	std::shared_ptr<Type> check(std::shared_ptr<Type> p1, std::shared_ptr<Type> p2) {
		if (Type::numeric(p1) && Type::numeric(p2)) return p2;
		else if (p1 == Type::Bool && p2 == Type::Bool) return p2;
		else return nullptr;
	}

	void gen(int b, int a) override {
		emit(id->toString() + " = " + expr->gen()->toString());
	}
};

/*
	Узел оператора присваивания для элемента массива
*/
class SetElem : public Stmt {
public:
	std::shared_ptr<Id> arr;
	std::shared_ptr<Expr> index;
	std::shared_ptr<Expr> expr;
	SetElem(std::shared_ptr<Access> x, std::shared_ptr<Expr> y) : arr(x->arr), index(x->index), expr(y) {
		if (check(x->type, y->type) == nullptr) error("type error");
	}

	json toJson() override {
		json a = arr->toJson();
		json b = index->toJson();
		json c = expr->toJson();

		json j = { { "name", "SetElem" }, {"children", { a, b, c }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"SetElem\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << arr->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << index->toDot(ss) << std::endl;
		ss << '\t' << i << " -> " << expr->toDot(ss) << std::endl;


		return i;
	}

	std::shared_ptr<Type> check(std::shared_ptr<Type> p1, std::shared_ptr<Type> p2) {
		Array* arr_left = dynamic_cast<Array*>(p1.get());
		Array* arr_right = dynamic_cast<Array*>(p2.get());
		if (arr_left || arr_right) return nullptr;
		else if (p1 == p2) return p2;
		else if (Type::numeric(p1) && Type::numeric(p2)) return p2;
		else return nullptr;
	}

	void gen(int b, int a) override {
		std::string s1 = index->reduce()->toString();
		std::string s2 = expr->reduce()->toString();

		emit(arr->toString() + " [ " + s1 + " ] = " + s2);
	}
};

/*
	Узел с последовательностью инструкций
*/
class Seq : public Stmt {
public:
	std::shared_ptr<Stmt> stmt1;
	std::shared_ptr<Stmt> stmt2;
	Seq(std::shared_ptr<Stmt> s1, std::shared_ptr<Stmt> s2) : stmt1(s1), stmt2(s2) {}

	json toJson() override {
		json s1 = stmt1->toJson();
		json s2 = stmt2->toJson();

		json j = { { "name", "Seq" }, {"children", { s1, s2 }} };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Seq\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		ss << '\t' << i << " -> " << stmt1->toDot(ss) << std::endl;;
		ss << '\t' << i << " -> " << stmt2->toDot(ss) << std::endl;;

		return i;
	}

	void gen(int b, int a) override {
		if (stmt1 == Stmt::Null) stmt2->gen(b, a);
		else if (stmt2 == Stmt::Null) stmt1->gen(b, a);
		else {
			int label = newlabel();
			stmt1->gen(b, label);
			emitlabel(label);
			stmt2->gen(label, a);
		}
	}
};

/*
	Узел оператора выхода из цикла
*/
class Break : public Stmt {
public:
	std::shared_ptr<Stmt> stmt;
	Break() {
		if (Stmt::Enclosing == nullptr) error("Unenclosed break");
		stmt = Stmt::Enclosing;
	}

	json toJson() override {
		json j = { { "name", "Break" } };
		return j;
	}

	uint32_t toDot(std::stringstream& ss) override {
		uint32_t i = hash(this);

		ss << '\t' << i << ' ' << "[shape=box, label=\"Break\", fillcolor=\"#e3f2fd\", style=filled]" << std::endl;

		return i;
	}
	
	void gen(int b, int a) override {
		std::stringstream ss;
		ss << "goto L" << stmt->after;
		emit(ss.str());
	}
};

