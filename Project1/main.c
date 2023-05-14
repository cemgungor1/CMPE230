#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define maxLen 256

// defined a struct to hold variable name and values
typedef struct
{
    char name[maxLen];
    long long int value;
} Variable;
// defined a struct to keep track of operations, operands, and parenthesis
typedef struct
{
    int operandTop;
    int operationTop;
    int parenthesisTop;
    long long int operands[maxLen];
    char parenthesis[maxLen];
    char operators[maxLen];
} Stack;
// a list to hold defined variables so that they are findable
Variable variableList[128];
int varCount = 0;
// defined necessary variables, most importantly a static const emptystack so that when an error occurs
// I don't have to recreate stack:operation
Stack operation;
static const Stack EmptyStack;
char syntaxCheck[128];
int syntaxLength;
int unaryCheck;
int errorCheck;

char popOperator(Stack *stack);
char popParenthesis(Stack *stack);
int getOperatorPriority(char op);
int isOperator(char op);
int varNameCheck(char *, char *);
int findVar(char *);
long long int evaluate(char *);
long long int applyOperator(Stack *stack);
long long int popOperand(Stack *stack);
long long int funcNameCheck(char *);
void pushOperand(Stack *stack, long long int operand);
void pushOperator(Stack *stack, char operator);
void pushParenthesis(Stack *stack);
void syntaxChecker();

int main()
{
    printf("> ");
    char inp[maxLen];
    while (fgets(inp, maxLen, stdin) != NULL)
    {
        // after each input, every variable that are used except variable list are being set to initial values
        operation = EmptyStack;
        operation.operandTop = -1;
        operation.operationTop = -1;
        operation.parenthesisTop = -1;
        unaryCheck = -1;
        errorCheck = 0;
        syntaxCheck[0] = '\0';
        syntaxLength = 0;

        inp[strcspn(inp, "\n")] = '\0';
        // if there's a comment, find the position of the comment and alter the inp so that
        // comment part is neglected
        if (strchr(inp, '%') != NULL)
        {
            char temp[maxLen];
            char *pos = strchr(inp, '%');
            strncpy(temp, inp, pos - inp);
            temp[pos - inp] = '\0';
            strcpy(inp, temp);
        }
        // if null input then print blank line and continue
        if (strcmp(inp, "") == 0)
        {
            printf("> ");
            continue;
        }
        // if there's no '=' then just evaluate the expression
        else if (strchr(inp, '=') == NULL)
        {
            long long int res = evaluate(inp);
            // no error and no input --> blank input then just print >
            if (syntaxLength == 0 && errorCheck != -1)
            {
                printf("> ");
                continue;
            }
            // if the expression doesn't start with variable integer or ( then syntax error
            else if (syntaxCheck[syntaxLength - 1] != 'v' && syntaxCheck[syntaxLength - 1] != 'i' && syntaxCheck[syntaxLength - 1] != ')')
            {
                errorCheck = -1;
            }
            // if no problem is found then print the result
            if (errorCheck != -1)
            {
                printf("%lld\n", res);
            }
            else
            {
                printf("Error!\n");
            }
        }
        else
        {
            // if there's = sign then split the input into half and then check if the variable exists while evaluating the expression
            char lhs[maxLen];
            char rhs[maxLen];
            char *pos = strchr(inp, '=');
            strncpy(lhs, inp, pos - inp);
            lhs[pos - inp] = '\0';
            strcpy(rhs, pos + 1);
            rhs[strcspn(rhs, "\n")] = '\0';
            varNameCheck(lhs, rhs);
        }
        printf("> ");
    }
    return 0;
}

char popOperator(Stack *stack)
{
    return stack->operators[stack->operationTop--];
}

char popParenthesis(Stack *stack)
{
    return stack->parenthesis[stack->parenthesisTop--];
}

int getOperatorPriority(char op)
{
    // just a simple function that returns operator priority, functions which are indicated using letters have the highest priority
    if (op == '|')
    {
        return 1;
    }
    else if (op == '&')
    {
        return 2;
    }
    else if (op == '+' || op == '-')
    {
        return 3;
    }
    else if (op == '*')
    {
        return 4;
    }
    else if (
        op == '|' || op == '^' ||
        op == 'N' || op == 'R' ||            // not : N left shift: S right shift: s
        op == 'r' || op == 'S' || op == 's') // left rotate: R right rotate: r
    {
        return 5;
    }
    else
    {
        return 0;
    }
}

int isOperator(char op)
{
    // simple function to check if a char is an operator, if the operator has unary minus or two operators follow each other
    // then change errorCheck var
    if ((op == '+' || op == '*' || op == '-' || op == '&' || op == '|') && unaryCheck != -1)
    {
        return 1;
    }
    else if ((op == '+' || op == '*' || op == '-' || op == '&' || op == '|') && unaryCheck == -1)
    {
        errorCheck = -1;
        return 0;
    }
    else
    {
        return 0;
    }
}

int varNameCheck(char *var, char *rhs)
{
    int length = strlen(var);
    int startOfVar = 0;
    int endOfVar = length - 1;
    // first find the indexes that don't have space character
    while (isspace(var[startOfVar]))
    {
        startOfVar++;
    }
    while (isspace(var[endOfVar]))
    {
        endOfVar--;
    }
    for (int indx = startOfVar; indx <= endOfVar; indx++)
    {
        // check if there's a space or nonalphabetic character in between, if so return error
        if (!isalpha(var[indx]) || isspace(var[indx]))
        {
            printf("Error!\n");
            errorCheck = -1;
            return 0;
        }
    }
    // if not create the varName variable, and copy the var into varName
    char varName[endOfVar - startOfVar + 2];
    strncpy(varName, var + startOfVar, endOfVar - startOfVar + 1);
    varName[endOfVar - startOfVar + 1] = '\0';
    // if its a function name then return error
    if (strcmp(varName, "xor") == 0 || strcmp(varName, "ls") == 0 ||
        strcmp(varName, "rs") == 0 || strcmp(varName, "lr") == 0 ||
        strcmp(varName, "rr") == 0 || strcmp(varName, "not") == 0)
    {
        errorCheck = -1;
        return 0;
    }
    long long int value = 0;
    value = evaluate(rhs);
    // first evaluate the rhs, then if there's no error check the index of the var
    // if there's no such variable, then create a new variable, else equalize the result
    // to that index
    if (errorCheck != -1)
    {
        int index;
        index = findVar(varName);
        if (index == -1)
        {
            strcpy(variableList[varCount].name, varName);
            variableList[varCount].value = value;
            varCount++;
        }
        else
        {
            variableList[index].value = value;
        }
    }
    else
    {
        printf("Error!\n");
    }
}

int findVar(char *name)
{
    // just a loop that checks if the name is in the variable list
    for (int i = 0; i < varCount; i++)
    {
        if (strcmp(variableList[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

long long int evaluate(char *inp)
{
    int length = strlen(inp);
    for (int i = 0; i < length; i++)
    {
        // start by checking every input, if error then return else go on
        // if inp[i] is variable:v, operator:o, comma:, paranthesis:() function:f integer:i we push to
        // syntaxCheck and call syntaxchecker
        if (errorCheck == -1)
        {
            return 0;
        }
        // if blank or tab, skip since not considered
        else if (inp[i] == ' ' || inp[i] == '\t')
        {
            continue;
        }

        else if (inp[i] == ',')
        {
            syntaxCheck[syntaxLength++] = ',';
            syntaxChecker();
            unaryCheck = -1;
            // a little problematic side but if comma then run the code till ( is found
            while (operation.operators[operation.operationTop] != '(' && errorCheck != -1)
            {
                long long int result = applyOperator(&operation);
                pushOperand(&operation, result);
            }
        }
        else if (inp[i] == '(')
        {
            pushParenthesis(&operation);
            syntaxCheck[syntaxLength++] = '(';
            syntaxChecker();
            unaryCheck = -1;
            pushOperator(&operation, inp[i]);
        }
        else if (inp[i] == ')')
        {
            // first check paranthesis count
            if (operation.parenthesisTop == -1)
            {
                errorCheck = -1;
                continue;
            }
            // if there isn't a problem while a ( is not encountered, apply operator
            popParenthesis(&operation);
            syntaxCheck[syntaxLength++] = ')';
            syntaxChecker();
            while (operation.operators[operation.operationTop] != '(' && errorCheck != -1)
            {
                long long int result = applyOperator(&operation);
                pushOperand(&operation, result);
            }
            // if encountered then pop the (
            if (operation.operators[operation.operationTop] == '(')
            {
                popOperator(&operation);
            }
        }
        else if (isdigit(inp[i]))
        {
            // since digit characters have values offset from 0 character we subtract 0 from
            // the found digit to turn into integer, then while the next input is digit
            // we do it in a loop
            syntaxCheck[syntaxLength++] = 'i';
            syntaxChecker();
            // unarycheck reset since digit is found
            unaryCheck = 0;
            long long int operand = inp[i] - '0';
            while (isdigit(inp[i + 1]))
            {
                operand = operand * 10 + (inp[i + 1] - '0');
                i++;
            }
            pushOperand(&operation, operand);
        }
        else if (isOperator(inp[i]))
        {
            syntaxCheck[syntaxLength++] = 'o';
            syntaxChecker();
            // if an operator is found, apply it if the stack is not empty, theres no error and
            // it has lower precedence
            while (operation.operationTop != -1 && errorCheck != -1 &&
                   getOperatorPriority(inp[i]) <= getOperatorPriority(operation.operators[operation.operationTop]))
            {
                long long int result = applyOperator(&operation);
                pushOperand(&operation, result);
            }
            unaryCheck = -1;
            pushOperator(&operation, inp[i]);
        }
        else if (isalpha(inp[i]))
        {
            // if alphabetic character, then reset unary since a function or variable is found
            // and then create the name till a space is found
            unaryCheck = 0;
            char varOrFunc[maxLen];
            int startI = i;
            int length = 0;
            while (isalpha(inp[i + 1]))
            {
                length++;
                i++;
            }
            strncpy(varOrFunc, inp + startI, length + 1);
            varOrFunc[length + 1] = '\0';
            // check the result of funcnamecheck, it returns -1 if the given variable is a function
            // else returns the value
            long long int result = funcNameCheck(varOrFunc);
            syntaxChecker();
            if (result == -1)
            {
                ;
            }
            else
            {
                pushOperand(&operation, result);
            }
        }
        else
        {
            errorCheck = -1;
            return 0;
        }
    }
    // if operation has a parenthesis left then theres an extra parenthesis so error
    if (operation.parenthesisTop != -1)
    {
        errorCheck = -1;
    }
    while (operation.operationTop != -1 && errorCheck != -1)
    {
        long long int result = applyOperator(&operation);
        pushOperand(&operation, result);
    }
    // if no error is found in all other operations and there's none operand left, then return
    // the result which is the last operand left, else return error.
    if (errorCheck != -1)
    {
        if (operation.operandTop != 0 && operation.operandTop != -1)
        {
            errorCheck = -1;
        }
        else
        {
            return popOperand(&operation);
        }
    }
    else
    {
        return 0;
    }
}

long long int applyOperator(Stack *stack)
{
    // just a basic operator applier, if the operator is not function(N) then dont
    // pop another operand since its not needed
    // check if operator is not n and there's not enough operands, then it should give an error
    char operator= popOperator(&operation);
    if (operation.operandTop == 0 && operator!= 'N')
    {
        errorCheck = -1;
    }
    long long int operand2 = popOperand(&operation);
    long long int operand1;
    if (operator!= 'N')
    {
        operand1 = popOperand(&operation);
    }
    else
    {
        ;
    }
    switch (operator)
    {
    case '+':
        return operand1 + operand2;
    case '*':
        return operand1 * operand2;
    case '-':
        return operand1 - operand2;
    case '&':
        return operand1 & operand2;
    case '|':
        return operand1 | operand2;
    case '^':
        return operand1 ^ operand2;
    case 'S':
        // checking shifts and rotates for negative second input, if so return error
        if (operand2 < 0)
        {
            errorCheck = -1;
            return -1;
        }
        else
        {
            return operand1 << operand2;
        }
    case 's':
        if (operand2 < 0)
        {
            errorCheck = -1;
            return -1;
        }
        else
        {
            return operand1 >> operand2;
        }
    case 'N':
        return ~operand2;
    case 'R':
        // rotate function using built-in shift and or functions
        // in left rotate we first shift left by operand2, if the number*2^operand2 is less than 2^63
        // that means we don't lose any bits. If the multiplication is greater than 2^63, then we right
        // shift the operand1 by 64-operand2 bits so that the lost bits appear on right side.
        // at last we or both numbers to mix them up
        if (operand2 < 0)
        {
            errorCheck = -1;
            return -1;
        }
        else
        {
            return (operand1 << operand2) | (operand1 >> (64 - operand2));
        }
    case 'r':
        if (operand2 < 0)
        {
            errorCheck = -1;
            return -1;
        }
        else
        {
            return (operand1 >> operand2) | (operand1 << (64 - operand2));
        }
    }
}

long long int popOperand(Stack *stack)
{
    return stack->operands[stack->operandTop--];
}

long long int funcNameCheck(char *var)
{
    // a function that checks if the string is a function or variable
    // adds the char to syntaxCheck in both cases and pushes operator if its a function
    if (strcmp(var, "xor") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, '^');
        return -1;
    }
    else if (strcmp(var, "ls") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, 'S');
        return -1;
    }
    else if (strcmp(var, "rs") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, 's');
        return -1;
    }
    else if (strcmp(var, "lr") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, 'R');
        return -1;
    }
    else if (strcmp(var, "rr") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, 'r');
        return -1;
    }
    else if (strcmp(var, "not") == 0)
    {
        syntaxCheck[syntaxLength++] = 'f';
        pushOperator(&operation, 'N');
        return -1;
    }
    else
    {
        syntaxCheck[syntaxLength++] = 'v';
        int index = findVar(var);
        if (index != -1)
        {
            return variableList[index].value;
        }
        else
        {
            return 0;
        }
    }
}

void pushOperand(Stack *stack, long long int operand)
{
    stack->operands[++stack->operandTop] = operand;
}

void pushOperator(Stack *stack, char operator)
{
    stack->operators[++stack->operationTop] = operator;
}

void pushParenthesis(Stack *stack)

{
    stack->parenthesis[++stack->parenthesisTop] = '(';
}

void syntaxChecker()
{
    // most of the syntax is checked using this function
    // for example after a variable:'v' only a closing paranthesis, comma or operator:'o' can occur
    char checkChar = syntaxCheck[syntaxLength - 1];
    // first, check the first element since the expression cant start with a comma for example
    if (syntaxLength == 1 && syntaxCheck[0] != 'v' && syntaxCheck[0] != 'i' && syntaxCheck[0] != 'f' && syntaxCheck[0] != '(')
    {
        errorCheck = -1;
    }
    // the compare element priort to last element to the last element
    else
    {
        switch (syntaxCheck[syntaxLength - 2])
        {
        case 'v':
            if (checkChar != ')' && checkChar != 'o' && checkChar != ',')
            {
                errorCheck = -1;
            }
            break;
        case 'i':
            if (checkChar != ')' && checkChar != 'o' && checkChar != ',')
            {
                errorCheck = -1;
            }
            break;
        case 'f':
            if (checkChar != '(')
            {
                errorCheck = -1;
            }
            break;
        case '(':
            if (checkChar == 'o' || checkChar == ',' || checkChar == ')')
            {
                errorCheck = -1;
            }
            break;
        case ')':
            if (checkChar != ')' && checkChar != 'o' && checkChar != ',')
            {
                errorCheck = -1;
            }
            break;
        case 'o':
            if (checkChar != 'v' && checkChar != 'i' && checkChar != 'f' && checkChar != '(')
            {
                errorCheck = -1;
            }

            break;
        case ',':
            if (checkChar != 'v' && checkChar != 'i' && checkChar != 'f' && checkChar != '(')
            {
                errorCheck = -1;
            }
            break;
        default:
            break;
        }
    }
}
