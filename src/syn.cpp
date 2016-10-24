#include <assert.h>
#include <algorithm>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include<list>
#include <map>
#include <string>
#include <vector>

struct evl_token
{
	enum token_type{NAME,NUMBER,SINGLE};
	token_type type;
	std::string str;
	int line_no;
}; //Structure Evl_token

typedef std::list<evl_token> evl_tokens;

struct evl_statement
{
	enum statement_type {MODULE,WIRE,COMPONENT,ENDMODULE};
	statement_type type;
	evl_tokens tokens;
}; //Structure evl_statement

typedef std::list<evl_statement> evl_statements;

struct evl_pin
{
	std::string name;
	int bus_msb, bus_lsb;
}; //Structure evl_pin

typedef std::list<evl_pin> evl_pins;

struct evl_wire
{
	std::string name;
	int width;
}; //Structure evl_wire

typedef std::list<evl_wire> evl_wires;

struct evl_module
{
	std::string name;
}; //Structure evl_module

typedef std::list<evl_module>evl_modules;

struct evl_component
{
	int NoPins;
	std::string type,name;
	evl_pins pins;
}; //Structure evl_component

typedef std::list<evl_component>evl_components;

bool extract_tokens_from_line(std::string line,int line_no, evl_tokens &tokens)
{
	for (size_t i = 0; i < line.size();)
	{
		if (line[i] == '/')
		{
			++i;
			if ((i == line.size()) || (line[i] != '/'))
			{
				std::cerr << "LINE " << line_no	<< ": a single / is not allowed" << std::endl;
				return false;
			}
			break;// skip the rest of the line by exiting the loop
		}
		else if (isspace(line[i]))
		{
		++i; // skip this space character
		}
		else if ((line[i] == '(') || (line[i] == ')')
			|| (line[i] == '[') || (line[i] == ']')
			|| (line[i] == ':') || (line[i] == ';')
			|| (line[i] == ',') || (line[i] == '=')
			|| (line[i] == '{') || (line[i] == '}'))	// implemented "{","}","=" as a SINGLE
		{
			evl_token token;
			token.line_no=line_no;
			token.type=evl_token::SINGLE;
			token.str=std::string(1, line[i]);
			tokens.push_back(token);
			++i;
		}
		else if (((line[i] >= 'a') && (line[i] <= 'z'))       // a to z
                        || ((line[i] >= 'A') && (line[i] <= 'Z'))    // A to Z
                        || (line[i] == '_'))
		{
			size_t name_begin = i;
			for (++i; i < line.size(); ++i)
			{
				if (!(((line[i] >= 'a') && (line[i] <= 'z'))
				   || ((line[i] >= 'A') && (line[i] <= 'Z'))
				   || ((line[i] >= '0') && (line[i] <= '9'))
				   || (line[i] == '_') || (line[i] == '$')))
				{
					break;	// [name_begin, i] is the range for the token
				}
			}
			evl_token token;
			token.line_no=line_no;
			token.type=evl_token::NAME;
			token.str=line.substr(name_begin, i-name_begin);
			tokens.push_back(token);
		}
		else if ((line[i] >= '0') && (line[i] <= '9')) // 0 to 9
		{
			size_t number_begin=i;
			for (++i; i<line.size();++i)
			{
				if(!((line[i] >= '0') && (line[i] <= '9')))
				{
					break; 	// [number_begin, i] is the range for the token
				}
			}
			evl_token token;
			token.line_no=line_no;
			token.type=evl_token::NUMBER;
			token.str=line.substr(number_begin,i-number_begin);
			tokens.push_back(token);
		}
		else
		{
			std::cerr << "LINE " << line_no	<< ": invalid character" << std::endl;
			return false;
		}
	}
	return true;
}

bool extract_tokens_from_file(std::string file_name, evl_tokens &tokens)
{
	std::ifstream input_file(file_name.c_str());
	if (!input_file)
	{
		std::cerr << "Cannot read file: " << file_name << "." << std::endl;
		return false;
	}
	tokens.clear();
	std::string line;
	for (int line_no = 1; std::getline(input_file, line); ++line_no)
	{
		if (!extract_tokens_from_line(line, line_no, tokens))
		{
			return false;
		}
	}
	return true;
}

bool store_tokens_to_file(std::string file_name, const evl_tokens &tokens)
{
		std::ofstream output_file(file_name.c_str());
		if (!output_file)
		{
			std::cerr << "Cannot write into file: " <<file_name << "."<< std::endl;
			return false;
		}
		return true;
}

bool token_is_semicolon(const evl_token &token)
{
	if(token.str == ";")
		return true;
	else
		return false;
}

bool move_tokens_to_statement(evl_tokens &statement_tokens,evl_tokens &tokens)
{
	assert(statement_tokens.empty());
	assert(!tokens.empty());
	evl_tokens::iterator next_sc = std::find_if(tokens.begin(), tokens.end(), token_is_semicolon);
	if (next_sc == tokens.end())
	{
		std::cerr << "Din't find ';' reached the end of line. Aborting!!!!" <<std::endl;
		return false;
	}
	++next_sc;
	statement_tokens.splice(statement_tokens.begin(),tokens, tokens.begin(), next_sc);
	return true;
}

bool group_tokens_into_statements(evl_statements &statements ,evl_tokens &tokens)
{
	assert(statements.empty());
	for (;!tokens.empty();)
	{
	// Generate one token per iteration
		evl_token token=tokens.front();
		if(token.type !=evl_token::NAME)
		{
			std::cerr<<"Need a NAME token but found '"<<token.str << "'on line"<<token.line_no<<std::endl;
			return false;
		}
		if (token.str == "module")
		{//module statement
			evl_statement module;
			module.type = evl_statement::MODULE;
			if (!move_tokens_to_statement(module.tokens, tokens))
				return false;
			statements.push_back(module);
		}
		else if (token.str == "endmodule")
		{//endmodule statement

			evl_statement endmodule;
			endmodule.type = evl_statement::ENDMODULE;
			endmodule.tokens.push_back(token);
			tokens.erase(tokens.begin());
			statements.push_back(endmodule);;
		}
		else if (token.str == "wire")
		{//wire statement
			evl_statement wire;
			wire.type = evl_statement::WIRE;
			if (!move_tokens_to_statement(wire.tokens,tokens))
				return false;
			statements.push_back(wire);
		}
		else
		{//component statement
			evl_statement component;
			component.type = evl_statement::COMPONENT;
			if (!move_tokens_to_statement(component.tokens,tokens))
				return false;
			statements.push_back(component);
		}
	}
	return true;
}

void display_statements(std::ostream &out,const evl_statements &statements,int val1, int val2, int val3)
{// creating display to write into ".statement" file
	int count = 1;
	for (evl_statements::const_iterator iter = statements.begin();	iter != statements.end(); ++iter, ++count)
	{
		if ((*iter).type == evl_statement::ENDMODULE)
		{
			out << "Statement " << count;
			out << ": ENDMODULE" << std::endl;
		}
		else if ((*iter).type == evl_statement::MODULE)
		{
			out << "Statement " << count;
			out << ": MODULE, " << val1 <<" tokens"<<std::endl;
		}
		else if ((*iter).type == evl_statement::WIRE)
		{
			out << "Statement " << count;
			out << ": Wire, " << val2 <<" tokens"<<std::endl;
		}
		else
		{
			out << "Statement " << count;
			out << ": COMPONENT, " <<val3 <<" tokens"<<std::endl;
		}
	}
}

bool store_statements_to_file(std::string file_name,const evl_statements &statements, int val1, int val2, int val3)
{//storing the above statements into a file "*.syntax"
	std::ofstream output_file(file_name.c_str());
	if (!output_file)
	{
		std::cerr << "Cannot write into file: " << file_name << "." << std::endl;
		return false;
	}
	display_statements(output_file, statements,val1,val2,val3);
	return true;
}

bool process_module_statement(evl_modules &modules,evl_statement &s)
{// logic for module statement
	assert(s.type == evl_statement::MODULE);
	evl_module module;
	for (; !s.tokens.empty(); s.tokens.pop_front())
	{
		evl_token t = s.tokens.front();
		if(t.type == evl_token::NAME)
		{
			module.name=t.str;
		}
		else
		{
			break;
		}
	}
	modules.push_back(module);
	return true;
}

int display_modules(std::ostream &out, const evl_modules &modules,const evl_wires &wires,const evl_components &components )
{
	for (evl_modules::const_iterator it = modules.begin();	it != modules.end(); ++it)
	{
		out << "module"  <<" "<< it->name<<" "<<std::endl;
	}
	return (3); //calculating the number of tokens
}

bool process_wire_statement(evl_wires &wires, evl_statement &s)
{// logic for wire statement
	assert(s.type == evl_statement::WIRE);
	enum state_type {INIT, WIRE, DONE, WIRES, WIRE_NAME, BUS,BUS_MSB, BUS_COLON, BUS_LSB, BUS_DONE};
	state_type state = INIT;
	int Bus_Width = 1;

	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front())
	{
		evl_token t = s.tokens.front();
		//  take one token at a time and state with 'INIT' state
		if (state == INIT)
		{
			if (t.str == "wire") {
				state = WIRE;
			}
			else {
				std::cerr << "Need 'wire' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRE)
		{
			if (t.type == evl_token::NAME)
			{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_Width;
				wires.push_back(wire);
				state = WIRE_NAME;
			}
			else if (t.str == "[")
			{
				state = BUS;
			}
			else {
				std::cerr << "Need NAME but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRES)
		{
                 	if (t.type == evl_token::NAME)
                 	{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_Width;
				wires.push_back(wire);
				state = WIRE_NAME;
			}
			else
			{
				std::cerr << "Need NAME but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRE_NAME)
		{
			if (t.str == ",")
			{
				state = WIRES;
			}
			else if (t.str == ";")
			{
				state = DONE;
			}
			else
			{
				std::cerr << "Need ',' or ';' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS)
		{
			if (t.type == evl_token::NUMBER)
			{
				Bus_Width = atoi(t.str.c_str())+1;
				state = BUS_MSB;
			}
			else
			{
				std::cerr << "Need NUMBER but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_MSB)
		{
			if (t.str == ":")
			{
				state = BUS_COLON;
			}
			else
			{
				std::cerr << "Need ':' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_COLON)
		{
			if (t.str == "0")
			{
				state = BUS_LSB;
			}
			else
			{
				std::cerr << "Need '0' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_LSB)
		{
			if (t.str == "]")
			{
				state = BUS_DONE;
			}
			else
			{
				std::cerr << "Need ']' but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_DONE)
		{
			if (t.type == evl_token::NAME)
			{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_Width;
				wires.push_back(wire);
				state = WIRE_NAME;
			}
			else
			{
				std::cerr << "Need NAME but found '" << t.str << "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else
		{
			assert(false);
		}
	}
	if (!s.tokens.empty() || (state != DONE))
	{
		std::cerr << "Wrong Statement!!!" << std::endl;
		return false;
	}
	return true;
}

int display_wires(std::ostream &out,const evl_wires &wires )
{
	out<<"wires"<<" "<<wires.size()<<std::endl;
	for (evl_wires::const_iterator it = wires.begin();it != wires.end(); ++it)
	{
		out << "  wire " << it->name<< " " << it->width<<std::endl;
	}
	return (1+(2*wires.size())); //calculating the number of tokens
}

bool process_component_statement(evl_components &components,evl_statement &s)
{// logic for component statement
	assert((!(s.type == evl_statement::WIRE)) &&(!(s.type == evl_statement::MODULE)) &&(!(s.type == evl_statement::ENDMODULE)));
	enum state_type {INIT, TYPE, NAME, PINS, PIN_NAME, BUS,BUS_MSB,BUS_COLON,BUS_LSB,BUS_DONE,PINS_DONE,DONE};
	state_type state = INIT;
	evl_component comp;
	evl_pin pin;

	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front())
	{
		evl_token t = s.tokens.front();
		if (state == INIT)
		{
			if (t.type==evl_token::NAME)
			{
				comp.type = t.str;
                              	comp.name = "";
                                  state = TYPE;
			}
			else {
				std::cerr << "Need NAME but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == TYPE)
		{
			if (t.type == evl_token::NAME)
			{
				comp.name = t.str;
				state = NAME;
			}
			else if (t.str == "(")
			{
				state = PINS;
			}
			else 
			{
				std::cerr << "Need NAME or '(' but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == NAME)
		{
			if (t.str == "(")
			{
				state = PINS;
			}
			else 
			{
				std::cerr << "Need '(' but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == PINS)
		{
			if (t.type == evl_token::NAME)
			{
				pin.name = t.str;
				pin.bus_msb = -1;
				pin.bus_lsb = -1;
				state = PIN_NAME;
			}
		else
			{
				std::cerr << "Need NAME but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == PIN_NAME)
		{	
			if (t.str == ",")
			{
				comp.pins.push_back(pin);
				state = PINS;
			}
			 else if (t.str == ")")
			{
				comp.pins.push_back(pin);
				state = PINS_DONE;
			}
			else if (t.str == "[")
			{
				state = BUS;
			}
			else
			{
				std::cerr << "Need ',' or ')' or '[' but found " << t.str<< "' on line " << t.line_no <<std::endl;
				return false;
			}

}
		else if (state == BUS)
		{
			if (t.type == evl_token::NUMBER)
			{
				pin.bus_msb = atoi(t.str.c_str());
				state = BUS_MSB;
			}
			else
			{
				std::cerr << "Need NUMBER but found '" << t.str	<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_MSB)
		{
			if (t.str == ":")
			{
				state = BUS_COLON;
			}
			else if (t.str == "]")
			{
				state = BUS_DONE;
			}
			else
			{
				std::cerr << "Need ':' or ']' but found " << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_COLON)
		{
			if (t.type == evl_token::NUMBER)
			{
				pin.bus_lsb = atoi(t.str.c_str());
				state = BUS_LSB;
			}
			else
			{
				std::cerr << "Need NUMBER but found '" << t.str	<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_LSB)
		{
			if (t.str == "]")
			{
				state = BUS_DONE;
			}
			else
			{
				std::cerr << "Need ']' but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS_DONE)
		{
			if (t.str == ")")
			{
				comp.pins.push_back(pin);
				state=PINS_DONE;
			}
			else if (t.str == ",")
			{
				comp.pins.push_back(pin);
				state = PINS;
			}
			else
			{
				std::cerr << "Need ')' or ',' but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
}
		else if (state == PINS_DONE)
		{
			if (t.str == ";")
			{
				state = DONE;
			}
			else
			{
			std::cerr << "Need ';' but found '" << t.str<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state ==DONE)
		{
			return true;
		}
		else
		{
			assert(false);
		}
	}
	components.push_back(comp);
	if (!s.tokens.empty() || (state != DONE))
	{
		std::cerr << "something wrong with the statement" << std::endl;
		return false;
	}
	return true;
}

int display_components(std::ostream &out,const evl_components &components )
{
	evl_components::const_iterator it = components.begin();
	out << "components " << std::distance(components.begin(),components.end()) << std::endl;
	for (;	it != components.end(); ++it)
	{
		if (it->name == "")
		{	out << "  component " <<it->type<< " "<<it->pins.size()<<std::endl;}
		else
			out << "  component " <<it->type<< " "<<it->name<<" "<<it->pins.size()<<std::endl;
		for(evl_pins::const_iterator iter=it-> pins.begin();iter!=it->pins.end();++iter)
		{
			if (iter->bus_msb == -1 && iter->bus_lsb ==-1)
			{
				out<<"    pin"<<" "<<iter->name<< std::endl;
			}
			else if (iter->bus_msb != -1 && iter->bus_lsb == -1)
			{
				out<<"    pin"<<" "<<iter->name<<*" "<<iter->bus_msb <<std::endl;
			}
			else
				out<<"    pin"<<" "<<iter->name<<*" "<<iter->bus_msb <<" "<<iter->bus_lsb<< std::endl;
		}
	}
	return(3+2*(components.size()));//calculating the number of tokens//not correct
}

int main(int argc, char *argv[])
{
	if (argc < 2)   // Input File 
	{
		std::cerr << "You should provide a file name." << std::endl;
		return -1;
	}

	std::string evl_file=argv[1];
	evl_tokens tokens;
	if (!extract_tokens_from_file(evl_file, tokens))\
	{
		return -1;
	}

	if(!store_tokens_to_file(evl_file+".tokens",tokens))
	{
		return -1;
	}

	evl_statements statements;
	if(!group_tokens_into_statements(statements,tokens)) 
	{
		return -1;
	}
	
	evl_wires wires;
	evl_components components;
	evl_modules modules;
	std::ofstream output_file((evl_file+ ".syntax").c_str()); //creating ".syntax" file
	for (evl_statements::iterator it=statements.begin();it!= statements.end(); ++it)
	{
		if ((*it).type == evl_statement::MODULE)
			{
				if(!process_module_statement(modules,(*it)))
				{
					return -1;
				}
			}
		else if ((*it).type == evl_statement::WIRE)
			{
				if(!process_wire_statement(wires,(*it)))
				{
					return -1;
				}
			}
		else if ((*it).type == evl_statement::COMPONENT)
			{
				if(!process_component_statement(components,(*it)))
				{
					return -1;
				}
			}
		else
			{
				break ;
			}
	}
	 //calculating the number of tokens and writing it to file
	int val1 = display_modules(output_file,modules,wires,components);
	int val2= display_wires(output_file,wires); 
	int val3 = display_components(output_file,components);
	
	if(!store_statements_to_file(evl_file+".statements",statements, val1, val2, val3)) 
	{
		return -1;
	}		
	return 0;
}
