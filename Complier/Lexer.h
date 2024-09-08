#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <cctype>
#include <memory>

enum Tag {
	AND = 256, BASIC = 257, BREAK = 258, DO = 259, 
	ELSE = 260, EQ = 261, FALSE = 262, GE = 263, 
	ID = 264, IF = 265, INDEX = 266, LE = 267,
	MINUS = 268, NE = 269, NUM = 270, OR = 271,
	REAL = 272, TEMP = 273, TRUE = 274, WHILE = 275
};

class Token {
public:
	Token(int t) : tag(t) {}
	int tag;
	virtual std::string toString() { 
		std::stringstream ss;
		ss << (char)tag;
		return ss.str(); 
	}
};

/*
	Integer number token
*/
class Num : public Token {
public:
	int value;
	Num(int v) : Token(NUM), value(v) {}
	std::string toString() override { 
		std::stringstream ss;
		ss << value;
		return ss.str();
	}
};

/*
	Token for keyword reserved identifiers
*/
class Word : public Token {
public:
	std::string lexeme;
	Word(std::string s, int tag) : Token(tag), lexeme(s) {}
	std::string toString() override { return lexeme; }

	static std::shared_ptr<Word> And;
	static std::shared_ptr<Word> Or;
	static std::shared_ptr<Word> Eq;
	static std::shared_ptr<Word> Ne;
	static std::shared_ptr<Word> Le;
	static std::shared_ptr<Word> Ge;
	static std::shared_ptr<Word> Minus;
	static std::shared_ptr<Word> True;
	static std::shared_ptr<Word> False;
	static std::shared_ptr<Word> Temp;
};

std::shared_ptr<Word> Word::And   = std::make_shared<Word>("&&", AND);
std::shared_ptr<Word> Word::Or    = std::make_shared<Word>("||", OR);
std::shared_ptr<Word> Word::Eq    = std::make_shared<Word>("==", EQ);
std::shared_ptr<Word> Word::Ne    = std::make_shared<Word>("!=", NE);
std::shared_ptr<Word> Word::Le    = std::make_shared<Word>("<=", LE);
std::shared_ptr<Word> Word::Ge    = std::make_shared<Word>(">=", GE);
std::shared_ptr<Word> Word::Minus = std::make_shared<Word>("minus", MINUS);
std::shared_ptr<Word> Word::True  = std::make_shared<Word>("true", TRUE);
std::shared_ptr<Word> Word::False = std::make_shared<Word>("false", FALSE);
std::shared_ptr<Word> Word::Temp  = std::make_shared<Word>("t", TEMP);

/*
	Floating point number token
*/
class Real : public Token {
public:
	float value;
	Real(float v) : Token(REAL), value(v) {}
	std::string toString() override {
		std::stringstream ss;
		ss << value;
		return ss.str();
	}
};


class Lexer {
public:
	Lexer(const char* filename) : peek(' ') {
		is.open(filename);

		if (!is.is_open()) {
			std::stringstream ss; ss << "Can not open file " << filename;
			throw std::exception(ss.str().c_str());
		}
	}

	static int line;
	char peek;
	std::map<std::string, std::shared_ptr<Word> > words;
	std::ifstream is;

	void reserve(std::shared_ptr<Word> w) { words.emplace(w->lexeme, w); }

	// Read next character from input stream
	void readch() {
		is.get(peek);
		if (is.eof()) peek = '\0';
		//std::cout << peek;
	}

	// Read next character from input stream and compare with c
	bool readch(char c) { 
		readch(); // TODO: check if it is correct
		if (peek != c) return false;
		peek = ' ';
		return true;
	}

	// Recognize next token
	std::shared_ptr<Token> scan() {

		// Skip whitespace characters
		for (;; readch()) {
			if (peek == ' ' || peek == '\t') continue;
			else if (peek == '\n') line++;
			else break;
		}

		// Recognize complex tokens that consist of two or more characters
		switch (peek)
		{
		case '&':
			if (readch('&')) return Word::And;
			else return std::make_shared<Token>('&');
			break;
		case '|':
			if (readch('"')) return Word::Or;
			else return std::make_shared<Token>('|');
			break;
		case '=':
			if (readch('=')) return Word::Eq;
			else return std::make_shared<Token>('=');
			break;
		case '!':
			if (readch('=')) return Word::Ne;
			else return std::make_shared<Token>('!');
			break;
		case '<':
			if (readch('=')) return Word::Le;
			else return std::make_shared<Token>('<');
			break;
		case '>':
			if (readch('=')) return Word::Ge;
			else return std::make_shared<Token>('>');
			break;
		default:
			break;
		}

		// Number recognition
		if (std::isdigit(peek)) {
			int v = 0;
			do {
				v = v * 10 + toDigit(peek); readch();
			} while (std::isdigit(peek));

			if (peek != '.') return  std::make_shared<Num>(v);
			float x = (float)v; float d = 10.f;

			for (;;) {
				readch();
				if (!std::isdigit(peek)) break;
				x += toDigit(peek) / d; d *= 10;
			}
			return std::make_shared<Real>(x);
		}

		// String recognition
		if (std::isalpha(peek)) {
			std::stringstream ss;
			do {
				ss << peek; readch();
			} while (std::isalpha(peek) || std::isdigit(peek));
			auto result = words.find(ss.str());
			if (result != words.end()) return result->second;
			std::shared_ptr<Word> w = std::make_shared<Word>(ss.str(), ID);
			words.emplace(ss.str(), w);
			return w;
		}

		// Other tokens recognition
		std::shared_ptr<Token> t = std::make_shared<Token>(peek);
		peek = ' ';
		return t;
	}

private:
	int toDigit(char c) {
		std::stringstream ss;
		ss << c;
		int d;
		ss >> d;
		return d;
	}
};

int Lexer::line = 0;