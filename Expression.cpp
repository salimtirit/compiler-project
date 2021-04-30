#include <iostream>
#include <stack>
#include <cstring>
#include <map>

using namespace std;
int lineNumber = 0;
map<string, int> variables; //name may ned to be changed due to conflicts
vector<string> declareStatements;
vector<string> otherStatements;

string operators = "-+()/*";
string bigger = "*/";
string smaller = "-+";

stack<string> expression;
stack<char> oper;
int namer = 0;
int chooseNamer = 0;
int condNamer = 0;
string evaluate(string expr);
void errorHandling(int line);

string varNamer(){
    return "_t" + to_string(namer++);
}

void declareVariable(string name, int value = 0)
{
    if (variables.find(name) == variables.end())
    {
        string str1 =  "%" + name + " = alloca i32";
        declareStatements.push_back(str1);
    }
    variables[name] = value;                              //  variables.insert(pair<string,int>(name,value));
    string str2 = "store i32 " + to_string(variables.find(name)->second) + ", i32* %" + name;
    if (value == 0)
    {
        declareStatements.push_back(str2);  // bura yanlış olabilir.
    }else{
        otherStatements.push_back(str2);
    }
    
}

bool isValidNumber(string s){
    
    for (int j = 0; j < s.length(); j++) {
        int character = s[j];
        if (!(character > 47 && character < 58)) //47 , 58 are the ascii table values
        {
            //no need to throw error message
            return false; 
        }
    }
    return true;
}

//returns true if the variable is valid
//valid variable: first character from alphabet (upper or lowercase) preceeding with alphanumeric characters

bool isValidVariable(string s, bool isTempVar = false)
{
 //   if(variables.find(s) == variables.end() && (variables.size()>1)){
   //     return true;
    //}

    if(isTempVar){
        return false;
    }

    int firstChr = s[0];
    if ((firstChr > 96 && firstChr < 123) || (firstChr > 64 && firstChr < 90)) // all ascii table values 
    {
        for (int i = 1; i < s.length(); i++)  {
            int character = s[i];
            if (!((character > 96 && character < 123) || (character > 64 && character < 90) || (character > 47 && character < 58))) {
                errorHandling(lineNumber);
            }
        }
        if (variables.find(s) == variables.end())  { //if it's not declared before
            declareVariable(s);
        }
        return true;
    } else {
        errorHandling(lineNumber);
    }
}

void chooseCondPrint(string expr, string oper, string condVar, string returnVar){
    condNamer++;
    string condName = "choose" + to_string(chooseNamer) + "cond" + to_string(condNamer); //choose0cond0
    string body = "choose" + to_string(chooseNamer) + "body" + to_string(condNamer); //choose0body0
    string chooseEnd = "choose" + to_string(chooseNamer) + "end" + to_string(condNamer); //choose0end0

    otherStatements.push_back("br label %" + condName);
    otherStatements.push_back(condName + ":");

    string boolVar = varNamer();
    otherStatements.push_back("%"+ boolVar + " = icmp " + oper + " i32 " + condVar + ", 0");
    otherStatements.push_back("br i1 %" + boolVar + ", label %" + body + ", label %"+ chooseEnd);
    otherStatements.push_back(body + ":");

    if(isValidNumber(expr)){
        otherStatements.push_back("store i32 "+ expr +", i32* %" + returnVar);
    } else { //expr2 will be given as a temp var, I want to give its value to another temp var which is returnVar
        otherStatements.push_back("store i32 "+ evaluate(expr) +", i32* %" + returnVar);
    }

    otherStatements.push_back("br label %"+chooseEnd);
    otherStatements.push_back(chooseEnd +":");
}



string choose(string var){
    
    var = var.substr(var.find("(")+1);  // deleting first "choose(" and ")" part
    var = var.substr(0, var.size()-1);
    vector<string> exprs;
    int parantheses = 0;
    string expr = "";

    for(char ch : var){
        if( ch == ',' && parantheses == 0){
            exprs.push_back(expr);
            expr = "";
        }else{   
            if(ch == '('){
                parantheses++;
            } else if(ch==')'){
                parantheses--;
            }
            expr = expr + ch;
        }
    }

    string expr4 = expr;
    string expr3 = exprs.back();
    exprs.pop_back();
    string expr2 = exprs.back();
    exprs.pop_back();   
    string expr1 = exprs.back();
    exprs.pop_back();
    string condVar;

    if(isValidNumber(expr1)){
        string tempRealVar = "%"+varNamer();
        declareStatements.push_back(tempRealVar + " = alloca i32");
        declareStatements.push_back("store i32 "+expr1+", i32* "+tempRealVar);
        condVar = "%" + varNamer();
        otherStatements.push_back(condVar + " = load i32* " + tempRealVar);
    } else {
        condVar = evaluate(expr1); //It returns a tempVar
    }
    
    string returnVar = varNamer();
    declareStatements.push_back("%" + returnVar + " = alloca i32");
    declareStatements.push_back("store i32 "+to_string(0)+", i32* %"+ returnVar);

    chooseCondPrint(expr2, "eq", condVar, returnVar);
    chooseCondPrint(expr3, "sgt", condVar, returnVar);
    chooseCondPrint(expr4, "slt", condVar, returnVar);

    chooseNamer++; //update
    condNamer = 0;

    string tempReturnVar = varNamer();
    otherStatements.push_back("%" + tempReturnVar + " = load i32* %" + returnVar);

    return "%"+tempReturnVar;
}

stack<string> postfix(string expr,stack<string> postfixExp){
    stack<string> revPostfix;
    string var = "";
    bool takingChoose = false; 
    string prevOper = "";
    string nestedChoose;
    int nested=0;
    int nestedParantheses = 0;
    for (char ch : expr) {

        if(takingChoose){
            
            if(ch != ')'){ //kalan ifadeyi almayı deniyorum
                if (ch == expr.back() && expr.find(ch)==(expr.size()-1) ) // ')' was expected, reached the end however
                {
                    errorHandling(lineNumber);
                }

                if(ch == '('){
                    if(var.find("choose")<var.size()){ //nested choose
                        nestedChoose = nestedChoose + var + ch;
                        var = "";

                        nested++;
                    } else {
                        nestedParantheses++;
                    }
                } else {
                var = var + ch;
                }
            } else { // ch == ')'
                if(nestedParantheses>0){
                    nestedParantheses--;
                }else{
                    if(nested){
                        var = var + ch;
                        nested--;
                        continue;
                    } else { // finished
                        var = "choose(" + nestedChoose + var + ")";
                        revPostfix.push(var);
                        var = "";
                        prevOper = "";
                        takingChoose = false;
                    }
                }
            }

        } else if (operators.find(ch) < operators.length()) { // ch bir operator

            if(ch == expr.back() && ch != ')' ){  // The last character of the string can not be an operator except ")", a variable is expected
                errorHandling(lineNumber); 
            }

            if(prevOper != "" ){  // only +( and )+ and )) and (( is valid, others are invalid
                if(ch == ')'){   //
                    if(prevOper != ")"){
                        errorHandling(lineNumber);
                    }
                } else if(prevOper == "("){
                    if(ch != '('){
                        errorHandling(lineNumber);
                    }
                }else if(prevOper != ")" && ch != '('){
                    errorHandling(lineNumber);  
                } 
            }
            string s(1,ch);
            prevOper = s;

            if(ch == '('){  // detecting choose
                if(var=="choose"){
                    var="";
                    takingChoose = true;
                    continue; //does this work?
                }
            }

            if (var != "") {
                revPostfix.push(var); //push prev var to postfix
                var = "";
            }

            if (oper.empty()) {    // if operator stack is empty
                if(ch == ')'){
                    otherStatements.push_back("'(' is expected, error in line: " +to_string(lineNumber));
                    //cout << "'(' is expected, error in line: " << lineNumber << endl; // testcase 4 de hata veriyor ya bu ya da alttaki 
                    errorHandling(lineNumber);
                }
                oper.push(ch); // directly put ch to stack
            } else { // stack boş değilse
                if (ch == ')') {
                    while (oper.top() != '(')
                    {
                        string opTop(1, oper.top());
                        revPostfix.push(opTop);
                        oper.pop();
                    
                        if(oper.empty()){
                        otherStatements.push_back("'(' is expected, error in line: " +to_string(lineNumber));
                        //cout << "'(' is expected, error in line: " << lineNumber << endl; // testcase 4 de hata veriyor
                        errorHandling(lineNumber); 
                        }
                    }                    
                    oper.pop();
                } else {
                    while (!oper.empty() && ch != '(' && oper.top() != '('){ //precedence
                        if (smaller.find(oper.top()) < 2 && bigger.find(ch) < 2){
                            break;
                        }
                        string opTop(1, oper.top());
                        revPostfix.push(opTop);
                        oper.pop();
                    }
                    oper.push(ch);
                }
            }

        } else { //ch is a variable
            var = var + ch;
            prevOper = "";
        }
    }

    if (var != "") {
        revPostfix.push(var); //push prev var to postfix
        var = "";
    }

    while(!oper.empty()){
        
        string opTop(1, oper.top());
        revPostfix.push(opTop);
        oper.pop();
    }

    while (!revPostfix.empty())
    {
        postfixExp.push(revPostfix.top()); 
        cout << revPostfix.top() << endl;
        revPostfix.pop();
    }
    return postfixExp;
}

string evaluate(string expr){
    stack<string> postfixExp;

    postfixExp = postfix(expr,postfixExp); //making new postfix expression
    if (postfixExp.size() == 1){   //if there is only a number inside just return number

        if(isValidNumber(postfixExp.top())){ //checks before its a valid number
            return postfixExp.top(); //->There should be something like declaereVariable ?
        } else {
            string singleVar;
            string loadVar;
            if(postfixExp.top().find("choose")<postfixExp.top().size()){
                return choose(postfixExp.top());
              //  loadVar = "_t" + to_string(namer++);
            } else if(isValidVariable(postfixExp.top())){
                singleVar = varNamer();
                otherStatements.push_back("%" + singleVar + " = load i32* %" + postfixExp.top());
                return "%" + singleVar;
            } else {
                errorHandling(lineNumber);
            }
        }
    }

    stack<pair<string,bool>> taken; 
    string returnVar = varNamer();
    declareVariable(returnVar);  // This will be returned

    while (!postfixExp.empty())  { // stack boş olana dek

        string s_top = postfixExp.top();

        if (!(operators.find(s_top) < operators.length()) || (s_top.find("choose(")<s_top.size())){ //s_top operator değil ise
            taken.push(make_pair(s_top,false)); // diğer stack'e at
            postfixExp.pop(); //I will check validity during operations since I need to know if it is a numb or a var

        } else { //s_top bir operator ise

            bool isTempVar1, isTempVar2;

            string var2 = taken.top().first;
            if(var2.find("choose(")<var2.size()){
                var2 = choose(var2);
                isTempVar2 = true;
            } else {
                isTempVar2 = taken.top().second;
            }
            taken.pop();
            
            string var1 = taken.top().first;
            if(var1.find("choose(")<var1.size()){
                var1 = choose(var1);
                isTempVar1 = true;
            } else {
                isTempVar1 = taken.top().second;
            }
            taken.pop();

            string operation;
            if(s_top == "+"){
                operation = "add";
            }else if(s_top == "-"){
                operation = "sub";    
            }else if(s_top == "*"){
                operation = "mul";    
            }else{
                operation = "sdiv";
            }

            string operand1, operand2;
            bool var1IsNumb = false;
            
            if(!isValidNumber(var1)){ 
                isValidVariable(var1,isTempVar1);
                 if(!isTempVar1){ 
                    operand1 = "%"+varNamer();
                    otherStatements.push_back( operand1 + " = load i32* %" + var1);
                 } else {
                     operand1 = var1;
                 }
            } else {
                otherStatements.push_back("store i32 "+ var1 +", i32* %" + returnVar);
                operand1 = "%"+varNamer();
                otherStatements.push_back( operand1 + " = load i32* %" + returnVar);
            }

            if(!isValidNumber(var2)){
                 isValidVariable(var2,isTempVar2);
                if(!isTempVar2){ 
                    operand2 = "%"+varNamer();
                    otherStatements.push_back( operand2 + " = load i32* %" + var2);
                } else {
                     operand2 = var2;
                }
            } else {
                operand2 = var2;
            }
      
            string pushVar = "%" + varNamer();
            otherStatements.push_back(pushVar+" = " + operation + " i32 " + operand1 + ", " + operand2);

            postfixExp.pop();
            taken.push(make_pair(pushVar,true));

        }
    }

    string topVar = taken.top().first;
    taken.pop();
    return topVar;
}
