#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define maxLen 256

typedef struct
{
    char name[maxLen];
    int value;
} Variable;
typedef struct
{
    int operandTop;
    int operationTop;
    int parenthesisTop;
    // operands is a 2d array having maxlen to maxlen, first is the count, second is the length
    char operands[maxLen][maxLen];
    char parenthesis[maxLen];
    char operators[maxLen];
} Stack;
Variable variableList[128];
int varCount = 0;
int lineCount = 0;
int fileLine = 1;
// files defined globally so that I can reach them inside functions. 
char *inputFile;
char *outputFile;
FILE *outputF;

Stack operation;
static const Stack EmptyStack;
char syntaxCheck[128];
int syntaxLength;
int unaryCheck;
int errorCheck;
// main error to check if an error occured while evaluating the expr, if so delete file.
int mainError;

char popOperator(Stack *stack);
char popParenthesis(Stack *stack);
int getOperatorPriority(char op);
int isOperator(char op);
int varNameCheck(char *, char *);
int findVar(char *);
char *evaluate(char *);
char *applyOperator(Stack *stack);
char *popOperand(Stack *stack);
int funcNameCheck(char *);
void pushOperand(Stack *stack, char operand[]);
void pushOperator(Stack *stack, char operator);
void pushParenthesis(Stack *stack);
void syntaxChecker();

// generally speaking most of the code is the same as advanced calculator, this time rather than using integers
// used chars for operands and rather than actually applying the operators, written it to a file, and used the line number
// in llvm file to add to the stack as the result.

int main(int argc, char *argv[])
{
    // this time since we are taking input file as parameter and output file as the same name as input file
    // created input and output files using args, used fputs to put the header lines to .ll file
    char inp[maxLen];
    mainError = 0;
    inputFile = argv[1];
    char *dot = strrchr(inputFile, '.');
    FILE *inputF;
    inputF = fopen(inputFile,"r");
    *dot = '\0';
    outputFile = malloc(strlen(inputFile) + 4);
    sprintf(outputFile, "%s.ll", inputFile);

    outputF = fopen(outputFile, "w");
    fputs("; ModuleID = 'advcalc2ir'\n",outputF);
    fputs("declare i32 @printf(i8*, ...)\n", outputF);
    fputs("@print.str = constant [4 x i8]  c\"%d\\0A\\00\"", outputF);
    fputs("\n\ndefine i32 @main() {\n", outputF);

    while (fgets(inp, sizeof(inp), inputF))
    {
        //line count is added to check which line we are currently on, so to print error
        lineCount += 1;
        operation = EmptyStack;
        operation.operandTop = -1;
        operation.operationTop = -1;
        operation.parenthesisTop = -1;
        unaryCheck = -1;
        errorCheck = 0;
        syntaxCheck[0] = '\0';
        syntaxLength = 0;
        // changed both \n and \r to \0 since it created problems
        inp[strcspn(inp, "\n")] = '\0';
        inp[strcspn(inp, "\r")] = '\0';
        if (strcmp(inp, "") == 0)
        {
            continue;
        }

        else if (strchr(inp, '=') == NULL)
        {
            // since we write to a file, evaluated expression using char arrays and pointes rather than integers
            // if there's a problem in the expression then didn't equalized it to res
            char res[32];
            char *nullCheck = evaluate(inp);
            if (nullCheck != NULL)
            {
                strcpy(res, nullCheck);
            }
            else
            {
                strcpy(res, "False");
            }

            if (syntaxLength == 0 && errorCheck != -1)
            {
                continue;
            }

            else if (syntaxCheck[syntaxLength - 1] != 'v' && syntaxCheck[syntaxLength - 1] != 'i' && syntaxCheck[syntaxLength - 1] != ')')
            {
                errorCheck = -1;
            }

            if (errorCheck != -1)
            {
                char outputLine[300];
                sprintf(outputLine, "\tcall i32 (i8*, ...) @printf(i8* getelementptr ([4 x i8], [4 x i8]* @print.str, i32 0, i32 0), i32 %%%i)\n", fileLine - 1);
                fputs(outputLine, outputF);
                fileLine++;
            }
            else
            {
                ;
            }
        }
        else
        {
            char lhs[maxLen];
            char rhs[maxLen];
            char *pos = strchr(inp, '=');
            strncpy(lhs, inp, pos - inp);
            lhs[pos - inp] = '\0';
            strcpy(rhs, pos + 1);
            rhs[strcspn(rhs, "\n")] = '\0';
            rhs[strcspn(rhs, "\r")] = '\0';
            varNameCheck(lhs, rhs);
        }
        if (errorCheck == -1){
            mainError = -1;
        }
    }
    // at last part closed inputfile, write a last line to make .ll file work and closed it also, but even if one 
    // line of the code had error, main error was set to -1, and if so removed outputfile.
    fclose(inputF);
    fputs("ret i32 0\n}", outputF);
    fclose(outputF);
    if (mainError == -1){
        remove(outputFile);
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
    // same get fct as the prior project, just added % and /
    if (op == '|')
    {
        return 1;
    }
    else if (op == '^')
    {
        return 2;
    }
    else if (op == '&')
    {
        return 3;
    }
    else if (op == '+' || op == '-')
    {
        return 4;
    }
    else if (op == '*' || op == '%' || op == '/')
    {
        return 5;
    }
    else if (
        op == 'N' || op == 'R' ||            // not : N left shift: S right shift: s
        op == 'r' || op == 'S' || op == 's') // left rotate: R right rotate: r
    {
        return 6;
    }
    else
    {
        return 0;
    }
}

int isOperator(char op)
{

    if ((op == '+' || op == '*' || op == '-' || op == '&' || op == '|' || op == '%' || op == '/') && unaryCheck != -1)
    {
        return 1;
    }
    else if ((op == '+' || op == '*' || op == '-' || op == '&' || op == '|' || op == '%' || op == '/') && unaryCheck == -1)
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
        // this time if variable name has space or something else other than alphabetical, then return error
        if (!isalpha(var[indx]) || isspace(var[indx]))
        {
            printf("Error in line %i \n", lineCount);
            errorCheck = -1;
            return 0;
        }
    }

    char varName[endOfVar - startOfVar + 2];
    strncpy(varName, var + startOfVar, endOfVar - startOfVar + 1);
    varName[endOfVar - startOfVar + 1] = '\0';

    if (strcmp(varName, "xor") == 0 || strcmp(varName, "ls") == 0 ||
        strcmp(varName, "rs") == 0 || strcmp(varName, "lr") == 0 ||
        strcmp(varName, "rr") == 0 || strcmp(varName, "not") == 0)
    {
        printf("Error in line %i \n", lineCount);
        errorCheck = -1;
        return 0;
    }
    // also checked the evaluate function, if theres an error, return error, if not then equalize value
    char value[32];
    char *nullCheck = evaluate(rhs);
    if (nullCheck == NULL)
    {
        errorCheck = -1;
        return 0;
    }

    strcpy(value, nullCheck);

    if (errorCheck != -1)
    {
        // if variable exist, then just store new value of the variable, if not allocate first then store
        char outputLine[300];
        int index;
        index = findVar(varName);
        if (index == -1)
        {
            sprintf(outputLine, "\t%%%s = alloca i32\n", varName);
            fputs(outputLine, outputF);
            strcpy(variableList[varCount].name, varName);
            sprintf(outputLine, "\tstore i32 %s, i32* %%%s\n", value, varName);
            fputs(outputLine, outputF);
            varCount++;
        }
        else
        {
            sprintf(outputLine, "\tstore i32 %s, i32* %%%s\n", value, varName);
            fputs(outputLine, outputF);
        }
    }
    else
    {
        ;
    }
}

int findVar(char *name)
{

    for (int i = 0; i < varCount; i++)
    {
        if (strcmp(variableList[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

char *evaluate(char *inp)
{
    // since I need to write to a file and use %(lineCount) rather than integers, I changed evaluate to return char
    // other than that most of the function works the same, it checks index by index, checks syntax,paranthesis, etc errors
    // if none occurs then evaluates using the stack, if error occurs prints error in line and returns null.
    int length = strlen(inp);
    for (int i = 0; i < length; i++)
    {
        if (errorCheck == -1)
        {
            printf("Error in line %d!\n", lineCount);
            return NULL;
        }

        else if (inp[i] == ' ' || inp[i] == '\t')
        {
            continue;
        }

        else if (inp[i] == ',')
        {
            syntaxCheck[syntaxLength++] = ',';
            syntaxChecker();
            unaryCheck = -1;

            while (operation.operators[operation.operationTop] != '(' && errorCheck != -1)
            {
                char result[32];
                strcpy(result, applyOperator(&operation));
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
            if (operation.parenthesisTop == -1)
            {
                errorCheck = -1;
                continue;
            }
            popParenthesis(&operation);
            syntaxCheck[syntaxLength++] = ')';
            syntaxChecker();
            while (operation.operators[operation.operationTop] != '(' && errorCheck != -1)
            {
                char result[32];
                strcpy(result, applyOperator(&operation));
                pushOperand(&operation, result);
            }

            if (operation.operators[operation.operationTop] == '(')
            {
                popOperator(&operation);
            }
        }
        else if (isdigit(inp[i]))
        {
            syntaxCheck[syntaxLength++] = 'i';
            syntaxChecker();
            unaryCheck = 0;
            int operand = inp[i] - '0';
            while (isdigit(inp[i + 1]))
            {
                operand = operand * 10 + (inp[i + 1] - '0');
                i++;
            }
            char number[32];
            sprintf(number, "%d", operand);
            pushOperand(&operation, number);
        }
        else if (isOperator(inp[i]))
        {
            syntaxCheck[syntaxLength++] = 'o';
            syntaxChecker();

            while (operation.operationTop != -1 && errorCheck != -1 &&
                   getOperatorPriority(inp[i]) <= getOperatorPriority(operation.operators[operation.operationTop]))
            {
                char result[32];
                strcpy(result, applyOperator(&operation));
                pushOperand(&operation, result);
            }
            unaryCheck = -1;
            pushOperator(&operation, inp[i]);
        }
        else if (isalpha(inp[i]))
        {
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

            int result = funcNameCheck(varOrFunc);
            syntaxChecker();
            if (result == -1 || result == 0)
            {
                ;
            }
            else
            {
                char res[32];
                sprintf(res, "%%%d", result);
                pushOperand(&operation, res);
            }
        }
        else
        {
            printf("Error in line %i!\n", lineCount);
            errorCheck = -1;
            return NULL;
        }
    }
    // since for loop finished, if any errors occur from the remaining stacks, then print those
    // if not then do the operations till it finishes.
    if (operation.parenthesisTop != -1)
    {
        printf("Error in line %i!\n", lineCount);
        errorCheck = -1;
        return NULL;
    }
    while (operation.operationTop != -1 && errorCheck != -1)
    {
        char result[32];
        strcpy(result, applyOperator(&operation));
        pushOperand(&operation, result);
    }

    if (errorCheck != -1)
    {
        if (operation.operandTop != 0 && operation.operandTop != -1)
        {
            printf("Error in line %i!\n", lineCount);
            errorCheck = -1;
            return NULL;
        }
        else
        {
            return popOperand(&operation);
        }
    }
    else
    {
        printf("Error in line %i!\n", lineCount);
        return NULL;
    }
}

char *applyOperator(Stack *stack)
{
    // the function that plays a huge role in functioning of the code. Uses pop operator and operands to look for the 
    // operation, switch-case between operations and if match is found, writes file the necessary output, 
    // increments fileLine, which checks the %(fileline) elements in llvm and returns the result line.
    static char result[32];
    char operator= popOperator(&operation);
    if (operation.operandTop == 0 && operator!= 'N')
    {
        errorCheck = -1;
    }
    char operand2[32];
    strcpy(operand2, popOperand(&operation));
    char operand1[32];
    char outputLine[300];
    if (operator!= 'N')
    {
        strcpy(operand1, popOperand(&operation));
    }
    else
    {
        ;
    }
    switch (operator)
    {
    case '+':
        sprintf(outputLine, "\t%%%i = add i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '*':
        sprintf(outputLine, "\t%%%i = mul i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '-':
        sprintf(outputLine, "\t%%%i = sub i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '/':
        sprintf(outputLine, "\t%%%i = sdiv i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '&':
        sprintf(outputLine, "\t%%%i = and i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '|':
        sprintf(outputLine, "\t%%%i = or i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '^':
        sprintf(outputLine, "\t%%%i = xor i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case '%':
        sprintf(outputLine, "\t%%%i = srem i32 %s, %s\n", fileLine, operand1, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case 'S':
        if (operand2 < 0)
        {
            errorCheck = -1;
            sprintf(result, "-1");
            return result;
        }
        else
        {
            sprintf(outputLine, "\t%%%i = shl i32 %s, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;
            sprintf(result, "%%%d", fileLine - 1);
            return result;
        }
    case 's':
        if (operand2 < 0)
        {
            errorCheck = -1;
            sprintf(result, "-1");
            return result;
        }
        else
        {
            sprintf(outputLine, "\t%%%i = ashr i32 %s, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;
            sprintf(result, "%%%d", fileLine - 1);
            return result;
        }
    case 'N':
        sprintf(outputLine, "\t%%%i = xor i32 %s, -1\n", fileLine, operand2);
        fputs(outputLine, outputF);
        fileLine++;
        sprintf(result, "%%%d", fileLine - 1);
        return result;
    case 'R':
        if (operand2 < 0)
        {
            errorCheck = -1;
            sprintf(result, "-1");
            return result;
        }
        else
        {
            // in order to make rotates work, first right shift then left shift then or line by line
            // separated the function operand1 << operand2 || operand1 >> (32-operand2)
            sprintf(outputLine, "\t%%%i = shl i32 %s, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(outputLine, "\t%%%i = ashr i32 %s, sub i32 32, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(outputLine, "\t%%%i = or i32 %%%i, %%%i\n", fileLine, fileLine - 1, fileLine - 2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(result, "%%%d", fileLine - 1);
            return result;
        }
    case 'r':
        if (operand2 < 0)
        {
            errorCheck = -1;
            sprintf(result, "-1");
            return result;
        }
        else
        {
            sprintf(outputLine, "\t%%%i = ashr i32 %s, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(outputLine, "\t%%%i = shl i32 %s, sub i32 32, %s\n", fileLine, operand1, operand2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(outputLine, "\t%%%i = or i32 %%%i, %%%i\n", fileLine, fileLine - 1, fileLine - 2);
            fputs(outputLine, outputF);
            fileLine++;

            sprintf(result, "%%%d", fileLine - 1);
            return result;
        }
    }
}

char *popOperand(Stack *stack)
{
    // operands are no longer integers so returns pointer
    stack->operandTop--;
    return stack->operands[stack->operandTop];
}

int funcNameCheck(char *var)
{
    // if function is found then add function to syntaxcheck and push operator
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
        // if a variable is found then load variable to memory, and write to file
        char outputLine[300];
        syntaxCheck[syntaxLength++] = 'v';
        int index = findVar(var);
        if (index != -1)
        {
            sprintf(outputLine, "\t%%%i = load i32, i32* %%%s\n", fileLine, variableList[index].name);
            fputs(outputLine, outputF);
            fileLine++;
            return fileLine - 1;
        }
        else
        {
            errorCheck = -1;
            return 0;
        }
    }
}

void pushOperand(Stack *stack, char operand[])
{
    // pushes the given operand to the given stack using strcpy.
    strcpy(stack->operands[stack->operandTop], operand);
    stack->operandTop++;
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
    // a syntax checker to check syntax of the given input, one by one, using the conditions
    // such as line cant end with an operator etc.
    char checkChar = syntaxCheck[syntaxLength - 1];

    if (syntaxLength == 1 && syntaxCheck[0] != 'v' && syntaxCheck[0] != 'i' && syntaxCheck[0] != 'f' && syntaxCheck[0] != '(')
    {
        errorCheck = -1;
    }
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
