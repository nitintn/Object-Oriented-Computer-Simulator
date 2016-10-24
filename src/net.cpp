#include <assert.h>
#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <stdexcept>
#include <map>

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

typedef std::map<std::string, int> evl_wires_table;
evl_wires_table make_wires_table(const evl_wires &wires);


//defining all the classes for netlist
class netlist;
class gate;
class net;
class pin;


class net{
public:
    	std::string n_name;
	std::list <pin *> connections_;
	std::map <std::string, net *> nets_table_;
	void append_pin(pin *);
}; //class net

class pin{
public:
    	std::string net_name;
    	int P_msb, P_lsb, length;
	gate *gate_;
	size_t pin_index_;
	std::vector <net *> nets_;
	bool create(gate *g, size_t pin_index, const evl_pin &p, const std::map<std::string, net *> &nets_table, const evl_wires_table &wires_table);
}; //class pin

class gate{
public:
    	std::string gate_type, gate_name;
	std::vector <pin *> pins_;
	typedef std::map <std::string, std::string> gates_table;
	gates_table gatespredef;
	bool create(const evl_component &component, const std::map <std::string, net *> &nets_table_, const evl_wires_table &wires_table);
	bool create_pin(const evl_pin &ep, size_t pin_index, const std::map<std::string, net *> &nets_table, const evl_wires_table &wires_table);
	bool validate_structural_semantics(std::string &gate_type, std::string &gate_name, const evl_pins &pins, gates_table &gatespredef);
}; //class gate

class netlist{
public:
	std::list <gate *> gates_;
	std::list <net *> nets_;
	std::string net_name;
	std::map <std::string, net *> nets_table_;

	bool create(const evl_wires &wires, const evl_components &components, const evl_wires_table &wires_table);
    	void display_netlist(std::ostream &out);

private:
	void create_net(std::string net_name);
	bool create_nets(const evl_wires &wires);
	bool create_gate(const evl_component &component, const evl_wires_table &wires_table);
	bool create_gates(const evl_components &components, const evl_wires_table &wires_table);
}; //class netlist

std::string make_net_name(std::string wire_name, int i);

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

void display_statements(std::ostream &out,const evl_statements &statements)
{
	int count = 1;
	for (evl_statements::const_iterator It = statements.begin();It != statements.end(); ++It, ++count) //right
		{if ((*It).type == evl_statement::ENDMODULE)
			{
				out << "statement " << count;
				out << ": ENDMODULE" << std::endl;
			}
			else if ((*It).type == evl_statement::MODULE)
			{
				out << "statement " << count;
				out << ": MODULE" << std::endl;
			}
			else if ((*It).type == evl_statement::WIRE)
			{
				out << "statement " << count;
				out << ": WIRE" << std::endl;
			}
			else //Remaining Component Module
			{
				out << "statement " << count;
				out << ": COMPONENT" << std::endl;

			}
		}
}

bool store_statements_to_file(std::string file_name,const evl_statements &statements)
{

	std::ofstream output_file(file_name.c_str());
	if (!output_file)
	{
		std::cerr << "I can't write into file " << file_name << "." << std::endl;
		return false;
	}
	display_statements(output_file, statements);
	return true;
}


bool process_module_statement(evl_modules &modules,evl_statement &n)
{
	assert(n.type == evl_statement::MODULE);
	evl_module module;
	for (; !n.tokens.empty(); n.tokens.pop_front())
	{
		evl_token t = n.tokens.front();

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

void display_modules(std::ostream &out, const evl_modules &modules)
{

	for (evl_modules::const_iterator it = modules.begin();	it != modules.end(); ++it)
	{
		out << "module"  <<" "<< it->name<<" "<< /*wires.size()<<" "<<comps.size()<<*/std::endl;
	}

}

bool process_wire_statement(evl_wires &wires, evl_statement &s)
{
	assert(s.type == evl_statement::WIRE);
	enum state_type {INIT, WIRE, DONE, WIRES, WIRE_NAME,BUS,BUS_MSB,BUS_COLON,BUS_LSB,BUS_DONE};

	state_type state = INIT;
	int Bus_length = 1;
	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front())
	{
		evl_token t = s.tokens.front();
		if (state == INIT)
		{
			if (t.str == "wire") {
				state = WIRE;
			}
			else {
				std::cerr << "Need 'wire' but found '" << t.str
					<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRE)
		{
			if (t.type == evl_token::NAME)
			{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_length;
				wires.push_back(wire);
				state = WIRE_NAME;
			}

			else if (t.str == "[")
			{
				state = BUS;
			}

			else {
				std::cerr << "Need NAME but found '" << t.str
					<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == WIRES)
		{
                 	if (t.type == evl_token::NAME)
                 	{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_length;
				wires.push_back(wire);
				state = WIRE_NAME;

			}

		else
			{
				std::cerr << "Need NAME but found '" << t.str
					<< "' on line " << t.line_no << std::endl;
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
				std::cerr << "Need ',' or ';' but found '" << t.str
					<< "' on line " << t.line_no << std::endl;
				return false;
			}
		}
		else if (state == BUS)
		{
			if (t.type == evl_token::NUMBER)
			{
				Bus_length = atoi(t.str.c_str())+1;
				state = BUS_MSB;
			}
			else
			{

			std::cerr << "Need NUMBER but found '" << t.str<< "' on line " << t.line_no << std::endl;
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
				std::cerr << "Need ':' but found '" << t.str<< "' on line " << t.line_no << std::endl;
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
				std::cerr << "Need '0' but found '" << t.str<< "' on line " << t.line_no << std::endl;
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
			if (t.type == evl_token::NAME)
			{
				evl_wire wire;
				wire.name =t.str;
				wire.width =Bus_length;
				wires.push_back(wire);
				state = WIRE_NAME;
			}
			else
			{
				std::cerr << "Need NAME but found '" << t.str<< "' on line " << t.line_no << std::endl;
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
		std::cerr << "something wrong with the Statement" << std::endl;
		return false;
	}
	return true;
}

void display_wires(std::ostream &out,const evl_wires &wires )
{
out<<"wires"<<" "<<wires.size()<<std::endl;
	for (evl_wires::const_iterator iter = wires.begin();iter != wires.end(); ++iter)
	{
		out << "  wire " << iter->name << " " << iter->width<<std::endl;
	}
}

bool process_Component_Statement(evl_components &components,evl_statement &s)
{
	assert((!(s.type == evl_statement::WIRE))&&(!(s.type == evl_statement::MODULE))&&(!(s.type == evl_statement::ENDMODULE)));
	enum state_type {INIT, TYPE, NAME, PINS, PIN_NAME,BUS,BUS_MSB,BUS_COLON,BUS_LSB,BUS_DONE,PINS_DONE,DONE};

	state_type state = INIT;
	evl_component cmp;
	evl_pin pin;
	//int no_ofpins=0;

	for (; !s.tokens.empty() && (state != DONE); s.tokens.pop_front())
	{
		evl_token t = s.tokens.front();
							//  Starts computation with INIT state
		if (state == INIT)
		{
			if (t.type==evl_token::NAME)
			{
				cmp.type = t.str;
                              	cmp.name = "";
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
				cmp.name = t.str;
				state = NAME;
			}
			else if (t.str == "(")
			{
				state = PINS;
			}
			else {
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
			else {
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

				cmp.pins.push_back(pin);
				state = PINS;
			}

			 else if (t.str == ")")
			{
				cmp.pins.push_back(pin);

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
				cmp.pins.push_back(pin);

				state=PINS_DONE;

			}
			else if (t.str == ",")
			{
				cmp.pins.push_back(pin);

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
	components.push_back(cmp);

	if (!s.tokens.empty() || (state != DONE))
	{
		std::cerr << "something wrong with the Statement" << std::endl;
		return false;
	}
	return true;
}

void display_components(std::ostream &out,const evl_components &components )
{
	evl_components::const_iterator iter = components.begin();
	out << "components " << std::distance(components.begin(),components.end()) << std::endl;
	for (;	iter != components.end(); ++iter)
	{
		if (iter->name == "")
		{
		out << "  component " <<iter->type<< " "<<iter->pins.size()<<std::endl;
		}
		else

		out << "  component " <<iter->type<< " "<<iter->name<<" "<<iter->pins.size()<<std::endl;
		for(evl_pins::const_iterator It=iter-> pins.begin();It!=iter->pins.end();++It)
		{
			if (It->bus_msb == -1 &&It->bus_lsb ==-1)
			{
			out<<"    pin"<<" "<<It->name<< std::endl;
			}
			else if (It->bus_msb != -1 &&It->bus_lsb == -1)
			{
                        out<<"    pin"<<" "<<It->name<<*" "<<It->bus_msb << std::endl;
			}
			else
                        out<<"    pin"<<" "<<It->name<<*" "<<It->bus_msb <<" "<<It->bus_lsb<< std::endl;
		}
	}
}

evl_wires_table make_wires_table(const evl_wires &wires) {
        evl_wires_table wires_table;
        for (evl_wires::const_iterator it = wires.begin(); it != wires.end(); ++it) {
                evl_wires_table::iterator same_name = wires_table.find(it->name);
                if(same_name != wires_table.end()){
                    std::cerr << "Wire '" << it->name << "' is already defined" << std::endl;
                    throw std::runtime_error("Multiple Wire Definitions");
                }
                wires_table.insert(std::make_pair(it->name, it->width));
        }
        return wires_table;
}

//netlist implementation start
std::string make_net_name(std::string wire_name, int i){
	assert(i >= 0);
	std::ostringstream oss;
	oss << wire_name << "[" << i << "]";
	return oss.str();
}

void netlist::create_net(std::string net_name){
	assert(nets_table_.find(net_name) == nets_table_.end());
	net *n = new net;
    	(*n).n_name = net_name;
	nets_table_[net_name] = n;
	nets_.push_back(n);
}

bool netlist::create_nets(const evl_wires &wires){
	for (evl_wires::const_iterator it = wires.begin(); it != wires.end(); it++){
		if (it->width == 1){
			create_net(it->name);
		}
		else{
			for (int i = 0; i < (it->width); ++i){
				create_net(make_net_name(it->name, i));
        	    	}
		}
	}
	return true;
}

void net::append_pin(pin *p) {
	connections_.push_back(p);
}

bool pin::create(gate *g, size_t pin_index, const evl_pin &p, const std::map<std::string, net *> &nets_table, const evl_wires_table &wires_table){
	pin_index_ = pin_index;
	gate_ = g;
  	P_msb = p.bus_msb;
 	P_lsb = p.bus_lsb;
    	net_name = p.name;

    	evl_wires_table::const_iterator itrwire = wires_table.find(net_name);

	if ((p.bus_msb == -1) && (p.bus_lsb == -1)){ // 1-bit wire in or bus in
		if(itrwire->second == 1){   // a 1-bit wire
			length = itrwire->second;
			net *netptr = new net;
			std::map<std::string, net *>::const_iterator nnameitr = nets_table.find(net_name);
			netptr = nnameitr->second;
			nets_.push_back(netptr);
			netptr->append_pin(this);
		}
    		else{
			length = itrwire->second;
			for(int i=0; i!=itrwire->second; ++i){
			    net *netptr = new net;
			    (*netptr).n_name = make_net_name(net_name, i);
			    std::map<std::string, net *>::const_iterator nnameitr = nets_table.find((*netptr).n_name);
			    netptr = nnameitr->second;
			    nets_.push_back(netptr);
			    netptr->append_pin(this);
			}
		}
	}
	else if ((p.bus_lsb != -1)&&(p.bus_msb != -1)){
		length = P_msb - P_lsb + 1;
		for(int i=P_lsb; i<=P_msb; ++i){
		    net *netptr = new net;
		    (*netptr).n_name = make_net_name(net_name, i);
		    std::map<std::string, net *>::const_iterator nnameitr = nets_table.find((*netptr).n_name);
		    netptr = nnameitr->second;
		    nets_.push_back(netptr);
		    netptr->append_pin(this);
		}
	}

	else if ((p.bus_msb != -1) && (p.bus_lsb == -1)){
		length = 1;
		net *netptr = new net;
		(*netptr).n_name = make_net_name(net_name, p.bus_msb);
		std::map<std::string, net *>::const_iterator nnameitr = nets_table.find((*netptr).n_name);
		netptr = nnameitr->second;
		nets_.push_back(netptr);
		netptr->append_pin(this);
	}
	return true;
}

bool gate::create_pin(const evl_pin &ep, size_t pin_index, const std::map<std::string, net *> &nets_table, const evl_wires_table &wires_table){

	pin *p = new pin;
	pins_.push_back(p);
	return p->create(this, pin_index, ep, nets_table, wires_table);
}

bool gate::create(const evl_component &component, const std::map <std::string, net *> &nets_table, const evl_wires_table &wires_table){
	gate_type = component.type;
	gate_name = component.name;
	size_t pin_index = 0;
	for (evl_pins::const_iterator it = component.pins.begin(); it != component.pins.end(); ++it){
		create_pin(*it, pin_index, nets_table, wires_table);
		++pin_index;
	}
 	return true;
}

bool netlist::create_gate(const evl_component &component, const evl_wires_table &wires_table){
	gate *g = new gate;
	gates_.push_back(g);
	return g->create(component, nets_table_, wires_table);
}

bool netlist::create_gates(const evl_components &components, const evl_wires_table &wires_table){
	for (evl_components::const_iterator itr = components.begin(); itr != components.end(); ++itr){
		create_gate(*itr, wires_table);
	}
	return true;
}

bool netlist::create(const evl_wires &wires, const evl_components &components, const evl_wires_table &wires_table){
	return create_nets(wires) && create_gates(components, wires_table);
}

void netlist::display_netlist(std::ostream &out){

	out << "nets " << nets_.size() << std::endl;
	for (std::list<net *>::const_iterator itnets = nets_.begin(); itnets != nets_.end(); ++itnets){
		out << "  net " << (*itnets)->n_name << " " << (*itnets)->connections_.size() << std::endl;
		for (std::list<pin *>::const_iterator itpins = (*itnets)->connections_.begin(); itpins != (*itnets)->connections_.end(); ++itpins){
			if ((*itpins)->gate_->gate_name == ""){
				out << "    " << (*itpins)->gate_->gate_type << " " << (*itpins)->pin_index_ << std::endl;
			}
			else{
				out << "    " << (*itpins)->gate_->gate_type << " " << (*itpins)->gate_->gate_name << " " << (*itpins)->pin_index_ << std::endl;
			}
		}
	}

	out << "components " << gates_.size() << std::endl;
	for (std::list<gate *>::const_iterator itgts = gates_.begin(); itgts != gates_.end(); ++itgts){
		if ((*itgts)->gate_name == ""){
		    out << "  component " << (*itgts)->gate_type << " " << (*itgts)->pins_.size() << std::endl;
		}
		else{
		    out << "  component " << (*itgts)->gate_type << " " << (*itgts)->gate_name << " " << (*itgts)->pins_.size() << std::endl;
		}
		for (std::vector<pin *>::const_iterator itrpins = (*itgts)->pins_.begin(); itrpins != (*itgts)->pins_.end(); ++itrpins){
			//out << "    pin " << "1 " << (*itrpins)->netname << std::endl;
            out << "    pin " << (*itrpins)->length;
            for(std::vector <net *>::const_iterator itrnets = (*itrpins)->nets_.begin(); itrnets != (*itrpins)->nets_.end(); ++itrnets){
                out << " " << (*itrnets)->n_name;
            }
            out << std::endl;
		}
	}
}

//netlist end

int main(int argc, char *argv[])
{
	if (argc < 2)   // Input File 
	{
		std::cerr << "You should provide a file name." << std::endl;
		return -1;
	}
	std::string evl_file=argv[1];
	evl_tokens tokens;
	if (!extract_tokens_from_file(evl_file, tokens))  
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
									
	if(!store_statements_to_file(evl_file+".statements",statements))
	{
		return -1;
	}

 	evl_components components;
	evl_wires wires;
	evl_modules modules;


	std::ofstream output_file((evl_file+ ".syntax").c_str());      //creating ".syntax" file
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

            	if(!process_Component_Statement(components,(*it)))
			{
				return -1;

			}
		}

		else
		{
			break ;

		}
	}

	display_modules(output_file,modules);
	display_wires(output_file,wires);
	display_components(output_file,components);

    	evl_wires_table wires_table = make_wires_table(wires);
    	netlist nl;

    	if (!nl.create(wires, components, wires_table)){
       		return -1;
    	}

    	std::ofstream outputfilenet((evl_file + ".netlist").c_str());//creating ".netlist" file
		display_modules(outputfilenet,modules);
		nl.display_netlist(outputfilenet);
	return 0;
}

