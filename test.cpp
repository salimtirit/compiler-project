#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <stack>
#include "Expression.cpp"
using namespace std;


int main(int argc, char const *argv[])
{
    string expr = "K+L-M*N+(O*P)W/U/V(T+Q)";
 
    stack<string> postfixExp  = postfix(expr);
   // evaluate(postfixExp);
    while(!postfixExp.empty()){
        cout << postfixExp.top();
        postfixExp.pop();
    }
    //KL+MN-OPWU/V/T*+Q+
    return 0;
}