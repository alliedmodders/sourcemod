#include <stdio.h>
#include "Lexer.h"
#include "Parser.h"

using namespace Knight;

int main()
{
	StringTable strtab;
	ErrorManager errors;
	Lexer k(&strtab, "public OnPluginStart() { new a; a = 5 + 6; }");
	Parser p(&k, &errors);

	p.ParseProgram();
}