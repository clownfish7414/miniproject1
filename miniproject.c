#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


// for lex
#define MAXLEN 256

// Token types
typedef enum {
    UNKNOWN, END, ENDFILE,
    INT, ID,
    ADDSUB, MULDIV,
    ASSIGN,
    LPAREN, RPAREN,
    AND, OR, XOR,
    INCDEC, ADDSUB_ASSIGN
} TokenSet;

TokenSet getToken(void);
TokenSet curToken = UNKNOWN;
char lexeme[MAXLEN];

// Test if a token matches the current token
int match(TokenSet token);
// Get the next token
void advance(void);
// Get the lexeme of the current token
char *getLexeme(void);
void settoken(void) {
    curToken = UNKNOWN;
}


// for parser
#define TBLSIZE 64
// Set PRINTERR to 1 to print error message while calling error()
// Make sure you set PRINTERR to 0 before you submit your code
#define PRINTERR 1

// Call this macro to print error message and exit the program
// This will also print where you called it in your program
#define error(errorNum) { \
    if (PRINTERR) \
        fprintf(stderr, "error() called at %s:%d: ", __FILE__, __LINE__); \
    err(errorNum); \
}

// Error types
typedef enum {
    UNDEFINED, MISPAREN, NOTNUMID, NOTFOUND, RUNOUT, NOTLVAL, DIVZERO, SYNTAXERR
} ErrorType;

// Structure of the symbol table
typedef struct {
    int val;
    char name[MAXLEN];
} Symbol;

// Structure of a tree node
typedef struct _Node {
    TokenSet data;
    int val;
    char lexeme[MAXLEN];
    struct _Node *left;
    struct _Node *right;
} BTNode;

int sbcount = 0;
Symbol table[TBLSIZE];

// Initialize the symbol table with builtin variables
void initTable(void);
// Get the value of a variable
int getval(char *str);
// Set the value of a variable
int setval(char *str, int val);
// Make a new node according to token type and lexeme
BTNode *makeNode(TokenSet tok, const char *lexe);
// Free the syntax tree
void freeTree(BTNode *root);
extern BTNode *factor(void);
extern void statement(void);
extern BTNode* assign_expr(void);
extern BTNode * or_expr(void);
extern BTNode * or_expr_tail(BTNode*left);
extern BTNode * xor_expr(void);
extern BTNode * xor_expr_tail(BTNode* left);
extern BTNode * and_expr(void);
extern BTNode * and_expr_tail(BTNode* left);
extern BTNode * addsub_expr(void);
extern BTNode * addsub_expr_tail(BTNode* left);
extern BTNode * muldiv_expr(void);
extern BTNode * muldiv_expr_tail(BTNode* left);
extern BTNode * unary_expr(void);
extern BTNode * factor(void);


// Print error message and exit the program
void err(ErrorType errorNum);


// for codeGen
// Evaluate the syntax tree
int rflag=0;
int ID_APPEAR = 0;
int evaluateTree(BTNode *root);
// Print the syntax tree in prefix
void printPrefix(BTNode *root);


/*============================================================================================
lex implementation
============================================================================================*/

TokenSet getToken(void)
{
    int i = 0;
    char c = '\0';

    while ((c = fgetc(stdin)) == ' ' || c == '\t');

    if (isdigit(c)) {
        lexeme[0] = c;
        c = fgetc(stdin);
        i = 1;
        while (isdigit(c) && i < MAXLEN) {
            lexeme[i] = c;
            ++i;
            c = fgetc(stdin);
        }
        ungetc(c, stdin);
        lexeme[i] = '\0';
        return INT;

    } 


   
    
    else if (c == '+' || c == '-') {

        
        lexeme[0] = c;
        c = fgetc(stdin);
        if (c == '+') {
            if (lexeme[0] == '+') {
                //printf("++\n");
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }
        }

        else if (c == '-') {
            if (lexeme[0] == '-') {
                //printf("--\n");
                lexeme[1] = c;
                lexeme[2] = '\0';
                return INCDEC;
            }
        }

        else if (c == '=') {
            //printf("+=\n");
            lexeme[1] = c;
            lexeme[2] = '\0';
            return ADDSUB_ASSIGN;

        }
        else {
            ungetc(c, stdin);
            lexeme[1] = '\0';
            return ADDSUB;
        }
        
        
    } else if (c == '*' || c == '/') {
        lexeme[0] = c;
        lexeme[1] = '\0';
        return MULDIV;
    } else if (c == '\n') {
        lexeme[0] = '\0';
        return END;
    } else if (c == '=') {
        strcpy(lexeme, "=");
        return ASSIGN;
    } else if (c == '(') {
        strcpy(lexeme, "(");
        return LPAREN;
    } else if (c == ')') {
        strcpy(lexeme, ")");
        return RPAREN;
    } else if (isalpha(c) || c=='_') {
        lexeme[0] = c;
        char nextchar = getchar();
        int index = 1;
        while (isalpha(nextchar) || nextchar == '_' || isdigit(nextchar) && index<MAXLEN) {
            lexeme[index] = nextchar;
            index += 1;
            nextchar = getchar();
        }
        ungetc(nextchar, stdin);
        lexeme[index] = '\0';
        return ID;
    } 
    else if (c == '&') {
        lexeme[0] = '&';
        lexeme[1] = '\0';
        return AND;
    }
    else if (c == '|') {
        lexeme[0] = '|';
        lexeme[1] = '\0';
        return OR;
    }
    else if (c == '^') {
        lexeme[0] = '^';
        lexeme[1] = '\0';
        return XOR;
    }
    
    else if (c == EOF) {
        return ENDFILE;
    } 
    else {
        return UNKNOWN;
    }
    
}

void advance(void) {
    curToken = getToken();
}

int match(TokenSet token) {
    if (curToken == UNKNOWN)
        advance();
    return token == curToken;
}



char *getLexeme(void) {
    return lexeme;
}

/*============================================================================================
parser implementation
============================================================================================*/

void initTable(void) {
    strcpy(table[0].name, "x");
    table[0].val = 0;
    strcpy(table[1].name, "y");
    table[1].val = 0;
    strcpy(table[2].name, "z");
    table[2].val = 0;
    sbcount = 3;
}

int getval(char *str) {
    int i = 0;
    //因為她會存入最新的一個就直接放rflag
    for (i = 0; i < sbcount; i++) {//改括號
        if (strcmp(str, table[i].name) == 0) {
            printf("[%d]", i * 4);
            
            return table[i].val;
        }
    }

    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    //new_variable += 1;
    err(NOTFOUND);
    
    strcpy(table[sbcount].name, str); //增加新變數
    printf("MOV r%d [%d]\n", rflag, i * 4);
    rflag += 1;
    table[sbcount].val = 0;
    sbcount++;
    return 0;
}

int setval(char *str, int val) {
    int i = 0;
    //printf("%s", str);

    for (i = 0; i < sbcount; i++) {
        if (strcmp(str, table[i].name) == 0) {
            
            
            table[i].val = val;

            printf("[%d]", i*4);
	
            return val;
        }
    }


    if (sbcount >= TBLSIZE)
        error(RUNOUT);
    
    strcpy(table[sbcount].name, str);
    table[sbcount].val = val;
    printf("[%d]", sbcount * 4);
    sbcount++;
    
    //if (rflag > 0) rflag--;//not sure
    return val;
}

BTNode *makeNode(TokenSet tok, const char *lexe) {
    BTNode* node = NULL;
    node=(BTNode*)malloc(sizeof(BTNode));
    strcpy(node->lexeme, lexe);
    node->data = tok;
    node->val = 0;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void freeTree(BTNode *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}



void statement(void) {
    BTNode* retp = NULL;

    if (match(ENDFILE)) {
        printf("MOV r0 [0]\n");
        printf("MOV r1 [4]\n");
        printf("MOV r2 [8]\n");
        printf("EXIT 0\n");
            
        exit(0);
    }
    else if (match(END)) {
        
        //printf(">> ");
        advance();
    }
    else {
        retp = assign_expr();
        if (match(END)) {
            //printf("%d\n", evaluateTree(retp));
            int num=evaluateTree(retp);
            //printf("num:%d", num);
            //printf("Prefix traversal: ");
            //printPrefix(retp);
            //printf("\n");
            freeTree(retp);
            //printf(">> ");
            advance();
        }
        else {
            error(SYNTAXERR);
        }
    }
}
//注意如果那麼早用如果是x++就在抓完x後會想去抓=或+=
    //應該要連用str2,str3,如果不對就unget
    //然後curtoken要設成unknown


    //運作過程 一開始main先statement()
    //如果讀到不是+=或=就ungets然後回到第一位
    //再往下跑一層邏輯再gettoken的時候就可以迴避掉那個問題了
extern BTNode* assign_expr(void) { 
    BTNode *retp,*left;
    char str2[60]={'\0'};
    char str3[60]={'\0'};
    if(match(ID)){
        left=makeNode(ID,getLexeme());
        advance();
        strcpy(str2,left->lexeme);
        strcpy(str3,getLexeme());
        if(match(END)) return left;
        else if(match(ASSIGN)){
            retp=makeNode(ASSIGN,getLexeme());
            advance();
            retp->left=left;
            retp->right=assign_expr();
        }
        else if (match(ADDSUB_ASSIGN)){
            retp=makeNode(ADDSUB_ASSIGN,getLexeme());
            advance();
            retp->left=left;
            retp->right=assign_expr();           
        }
        else{
            for(int i=strlen(str3)-1;i>=0;i--) ungetc(str3[i],stdin);       
            for(int i=strlen(str2)-1;i>=0;i--) ungetc(str2[i],stdin);     
            settoken();
            return or_expr();
        }
        return retp;
    }
    else return or_expr();
}


extern BTNode* or_expr(void) {
    BTNode* node = xor_expr();
    return or_expr_tail(node);

}


extern BTNode* or_expr_tail(BTNode* left) {
    BTNode* node = NULL;
    if (match(OR)) {
        node = makeNode(OR, getLexeme());
        advance();
        node->left = left;
        node->right = xor_expr();
        return or_expr_tail(node);

    }
    else {
        return left;
    }

}
extern BTNode* xor_expr(void) {
    BTNode* node = and_expr();
    return xor_expr_tail(node);
}

extern BTNode* xor_expr_tail(BTNode* left) {
    BTNode* node = NULL;
    if (match(XOR)) {
        node = makeNode(XOR, getLexeme());
        advance();
        node->left = left;
        node->right = and_expr();
        return xor_expr_tail(node);

    }
    else {
        return left;
    }

}


extern BTNode* and_expr(void) {
    BTNode* node = addsub_expr();
    return and_expr_tail(node);

}


extern BTNode* and_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(AND)) {
        node = makeNode(AND, getLexeme());
        advance();
        node->left = left;
        node->right = addsub_expr();
        return and_expr_tail(node);
    }
    else {
        return left;
    }

}


extern BTNode* addsub_expr(void) {
    BTNode* node = muldiv_expr();
    return addsub_expr_tail(node);

}


extern BTNode* addsub_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(ADDSUB)) {
        node = makeNode(ADDSUB, getLexeme());
        advance();
        node->left = left;
        node->right = muldiv_expr();
        return addsub_expr_tail(node);
    }
    else {
        return left;
    }

}


extern BTNode* muldiv_expr(void) {
    BTNode* node = unary_expr();
    return muldiv_expr_tail(node);

}

extern BTNode* muldiv_expr_tail(BTNode* left) {
    BTNode* node = NULL;

    if (match(MULDIV)) {
        node = makeNode(MULDIV, getLexeme());
        advance();
        node->left = left;
        node->right = unary_expr();
        return muldiv_expr_tail(node);
    }
    else {
        return left;
    }

}


extern BTNode* unary_expr(void) {
    BTNode* retp = NULL;
    if (match(ADDSUB)) {
        retp = makeNode(ADDSUB, getLexeme());
        advance();
        retp->left = makeNode(INT, "0");
        
        

        
        retp->right = unary_expr();
        
        
    }
    else {
        return factor();
    }
    return retp;
}
extern BTNode* factor(void) {
    BTNode* retp = NULL;

    if (match(INT)) {
        retp = makeNode(INT, getLexeme());
        advance();
    }
    else if (match(ID)) {
        retp = makeNode(ID, getLexeme());
        advance();
    }
    else if (match(INCDEC)) {
        retp = makeNode(INCDEC,getLexeme());
        //printf("%s", retp->lexeme);
        advance();
        if (match(ID)) {
            retp->left = makeNode(ID, getLexeme());
            //printf("%s", retp->left->lexeme);
            advance();
            retp->right = makeNode(INT, "1");
            
            //printf("finish");
        }
        else {
            error(UNDEFINED);
        }
    }
    
    else if (match(LPAREN)) {
        advance();
        retp = assign_expr();
        if (match(RPAREN))
            advance();
        else
            error(MISPAREN);
    }
    else {
        error(NOTNUMID);
    }
    return retp;

}




void err(ErrorType errorNum) {
    if (PRINTERR) {
        printf("EXIT 1\n");
        
    }
    exit(0);
}


/*============================================================================================
codeGen implementation
============================================================================================*/

int evaluateTree(BTNode *root) {
    int retval = 0, lv = 0, rv = 0;
    //static int rflag=0;
    //new_variable=0;
    

    if (root != NULL) {
        switch (root->data) {
            case ID:
                ID_APPEAR += 1;
                //printf("ID_APPEAR:%d\n",ID_APPEAR);
                //printf("ID\n");
                printf("MOV r%d ", rflag);
                rflag+=1;
                retval = getval(root->lexeme);
                printf("\n");
                break;
            case INT:
                //printf("INT\n");
                retval = atoi(root->lexeme);
                printf("MOV r%d %d\n",rflag,retval);
                rflag += 1;
                break;
            case ASSIGN:
                 
                //printf("ASSIGN\n");
                
                rv = evaluateTree(root->right);
                printf("MOV ");
                
                
                //printf("total%d\n", rv);
                retval = setval(root->left->lexeme, rv);
                printf(" r%d\n",rflag-1);
                
                break;
            case ADDSUB:
            case MULDIV:
                
                //printf("arithmatic\n");
                lv = evaluateTree(root->left);
                
                rv = evaluateTree(root->right);
                //printf("new_variable:%d\n",new_variable);
                
                if (strcmp(root->lexeme, "+") == 0) {
                    retval = lv + rv;
                    printf("ADD r%d r%d\n", rflag-2, rflag-1);
                    rflag -= 1;
                } else if (strcmp(root->lexeme, "-") == 0) {
                    retval = lv - rv;
                    printf("SUB r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                } else if (strcmp(root->lexeme, "*") == 0) {
                    retval = lv * rv;
                    printf("MUL r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                } else if (strcmp(root->lexeme, "/") == 0) {
                    if (rv == 0) {   
                        //printf("ID_APPEAR:%d\n",ID_APPEAR); 
                        
                        if (ID_APPEAR==0){
                            err(DIVZERO);
                        }
                        else {
                            printf("DIV r%d r%d\n", rflag - 2, rflag - 1);
                            rflag -= 1;
                            retval = 0;
                        }
                    }  
                    else {
                        printf("DIV r%d r%d\n", rflag - 2, rflag - 1);
                        rflag -= 1;
                        retval = lv / rv;
                    }
                }
                break;

            case INCDEC:
                
                //printf("INDEC\n");
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                
                if (strcmp(root->lexeme, "++") == 0) {
                    printf("ADD r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    printf("MOV ");
                    retval = lv + rv;
                    retval = setval(root->left->lexeme, retval);
                    printf(" r%d\n",rflag-1);
                }
                else if (strcmp(root->lexeme, "--") == 0) {
                    printf("SUB r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    printf("MOV ");
                    retval = lv - rv;
                    retval = setval(root->left->lexeme, retval);
                    printf(" r%d\n",rflag-1);
                }
                break;
                
            case AND:
            case OR:
            case XOR:
                
                //printf("logic\n");
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                
                if (strcmp(root->lexeme, "&") == 0) {
                    printf("AND r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    retval = lv & rv;
                }
                else if (strcmp(root->lexeme, "|") == 0) {
                    printf("OR r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    retval = lv | rv;
                }
                else if (strcmp(root->lexeme, "^") == 0) {
                    printf("XOR r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    retval = lv ^ rv;
                }
                
                break;


            case ADDSUB_ASSIGN:
                
                //printf("addsubassign\n");
                lv = evaluateTree(root->left);
                rv = evaluateTree(root->right);
                
                if (strcmp(root->lexeme, "+=") == 0) {
                    //getval(root->left->lexeme);
                    printf("ADD r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    printf("MOV ");
                    
                    rv = lv + rv;
                    //printf("\ntotal:%d\n", rv);
                    retval = setval(root->left->lexeme, rv);
                    printf(" r%d\n",rflag-1);
                }
                else if (strcmp(root->lexeme, "-=") == 0) {
                    printf("SUB r%d r%d\n", rflag - 2, rflag - 1);
                    rflag -= 1;
                    printf("MOV ");
                    rv = lv - rv;
                    //rv = rv - evaluateTree(root->right);
                    retval = setval(root->left->lexeme, rv);
                    printf(" r%d\n",rflag-1);
                }

                break;

            default:
                retval = 0;
        }
    }
    return retval;
}

void printPrefix(BTNode *root) {
    if (root != NULL) {
        printf("%s ", root->lexeme);
        printPrefix(root->left);
        printPrefix(root->right);
    }
}


/*============================================================================================
main
============================================================================================*/



int main() {
    initTable();
    //printf(">> ");
    while (1) {
        rflag=0;
        ID_APPEAR = 0;
        statement();
    }
    return 0;
}
