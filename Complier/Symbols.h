#pragma once
#include "Lexer.h"

/*
	Data Types Token
*/
class Type : public Word {
public:
	Type(std::string s, int tag, int w) : Word(s, tag), width(w) {}
	int width;
	static std::shared_ptr<Type> Int;
	static std::shared_ptr<Type> Float;
	static std::shared_ptr<Type> Char;
	static std::shared_ptr<Type> Bool;

	static bool numeric(std::shared_ptr<Type> p) {
		if (p == Char || p == Int || p == Float) return true;
		else return false;
	}

	static std::shared_ptr<Type> max(std::shared_ptr<Type> p1, std::shared_ptr<Type> p2) {
		if (!numeric(p1) || !numeric(p2)) return nullptr;
		if (p1 == Float || p2 == Float) return Float;
		else if (p1 == Int || p2 == Int) return Int;
		else return Char;
	}
};

std::shared_ptr<Type> Type::Int   = std::make_shared<Type>("int", BASIC, 4);
std::shared_ptr<Type> Type::Float = std::make_shared<Type>("float", BASIC, 8);
std::shared_ptr<Type> Type::Char  = std::make_shared<Type>("char", BASIC, 1);
std::shared_ptr<Type> Type::Bool  = std::make_shared<Type>("bool", BASIC, 1);

/*
	Data Array Token
*/
class Array : public Type {
public:
	std::shared_ptr<Type> of;
	int size;
	Array(int sz, std::shared_ptr<Type> p) : Type("[]", INDEX, sz * p->width), size(sz), of(p) {}
	std::string toString() override { std::stringstream ss; ss << '[' << size << ']' << of->toString(); return ss.str(); }

};