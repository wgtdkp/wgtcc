#include "lexer.h"

#include "error.h"

#include <cctype>
#include <set>

using namespace std;


/*
 * Return: true, is integer
 */
static bool ReadConstant(char*& p)
{
    bool sawEP = false;
    bool sawSign = false;
    bool sawDot = false;
    
    for (; 0 != p[0]; p++) {
        if (isxdigit(p[0]))
            continue;

        switch (p[0]) {
        //case '0' ... '9': 
        //case 'a' ... 'f': case 'A' ... 'F':
        case '.':
            sawDot = true;
        case 'u':
        case 'U':
        case 'l':
        case 'L':
        case 'f':
        case 'F':
        case 'x':
        case 'X': 
            break;
        case 'e':
        case 'E':
        case 'p':
        case 'P':
            sawEP = true;
            break;
        case '+':
        case '-':
            if (!sawEP || sawSign)
                return !(sawDot || sawEP);
            sawSign = true;
            break;
        default:
            return !(sawDot || sawEP);
        }
    }

    return !(sawDot || sawEP);
}

/**
O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\n\f]

{HP}{H}+{IS}?				{ return I_CONSTANT; }
{NZ}{D}*{IS}?				{ return I_CONSTANT; }
"0"{O}*{IS}?				{ return I_CONSTANT; }
{CP}?"'"([^'\\\n]|{ES})+"'"		{ return I_CONSTANT; }

{D}+{E}{FS}?				{ return F_CONSTANT; }
{D}*"."{D}+{E}?{FS}?			{ return F_CONSTANT; }
{D}+"."{E}?{FS}?			{ return F_CONSTANT; }
{HP}{H}+{P}{FS}?			{ return F_CONSTANT; }
{HP}{H}*"."{H}+{P}{FS}?			{ return F_CONSTANT; }
{HP}{H}+"."{P}{FS}?			{ return F_CONSTANT; }
*/

static inline bool IsBlank(char ch)
{
    return ' ' == ch || '\t' == ch || '\v' == ch || '\f' == ch;
}

void Lexer::Tokenize(TokenList& tokList)
{
    char* p = _text;
    
    Token tok;
    tok._tag = Token::END,
    tok._fileName = _fileName,
    tok._line = 1,
    tok._column = 1,
    tok._lineBegin = _text,
    tok._begin = _text,
    tok._end = _text;

    while (true) {
        while (IsBlank(p[0]) || '\n' == p[0]) {
            if ('\n' == p[0]) {
                if ('\r' == p[1]) {
                    ++p;
                }
                tok._lineBegin = p + 1;
                ++tok._line;
            }
            ++p;
        }

        if (p[0] == 0) {
            tok._end = p;
            tok._tag = Token::END;
            tokList.push_back(tok);
            return;
        }
        
        tok._tag = Token::END;
        
        tok._begin = p;
        switch (p[0]) {
        case '-':
            if ('>' == p[1]) {
                tok._tag = Token::PTR_OP; ++p;
            } else if ('-' == p[1]) {
                tok._tag = Token::DEC_OP; ++p;
            } else if ('=' == p[1]) {
                tok._tag = Token::SUB_ASSIGN; ++p;
            } else {
                tok._tag = Token::SUB;
            }
            ++p; break;

        case '+':
            if ('+' == p[1]) {
                tok._tag = Token::INC_OP; ++p;
            } else if ('=' == p[1]) {
                tok._tag = Token::ADD_ASSIGN; ++p;
            } else {
                tok._tag = Token::ADD;
            }
            ++p; break;

        case '<':
            if ('<' == p[1]) {
                if ('=' == p[2]) {
                    tok._tag = Token::LEFT_ASSIGN; ++p; ++p;
                } else {
                    tok._tag = Token::LEFT_OP; ++p;
                }
            } else if ('=' == p[1]) {
                tok._tag = Token::LE_OP; ++p;
            } else if (':' == p[1]) {
                tok._tag = '['; ++p;
            } else if ('%' == p[1]) {
                tok._tag = '{'; ++p;
            } else {
                tok._tag = Token::LESS;
            }
            ++p; break;

        case '>':
            if ('>' == p[1]) {
                if ('=' == p[2]) {
                    tok._tag = Token::RIGHT_ASSIGN; ++p; ++p;
                } else {
                    tok._tag = Token::RIGHT_OP; ++p;
                }
            } else if ('=' == p[1]) {
                tok._tag = Token::GE_OP; ++p;
            } else {
                tok._tag = Token::GREATER;
            }
            ++p; break;

        case '=':
            if ('=' == p[1]) {
                tok._tag = Token::EQ_OP; ++p;
            } else {
                tok._tag = Token::EQUAL;
            }
            ++p; break;

        case '!':
            if ('=' == p[1]) {
                tok._tag = Token::NE_OP; ++p;
            } else {
                tok._tag = Token::NOT;
            }
            ++p; break;

        case '&':
            if ('&' == p[1]) {
                tok._tag = Token::AND_OP; ++p;
            } else if ('=' == p[1]) {
                tok._tag = Token::AND_ASSIGN; ++p;
            } else {
                tok._tag = Token::AND;
            }
            ++p; break;

        case '|':
            if ('|' == p[1]) {
                tok._tag = Token::OR_OP; ++p;
            } else if ('=' == p[1]) {
                tok._tag = Token::OR_ASSIGN; ++p;
            } else {
                tok._tag = Token::OR;
            }
            ++p; break;

        case '*':
            if ('=' == p[1]) {
                tok._tag = Token::MUL_ASSIGN; ++p;
            } else {
                tok._tag = Token::MUL;
            }
            ++p; break;

        case '/':
            if ('/' == p[1]) {
                while ('\n' != p[0])
                    ++p;
                    
                tok._tag = Token::IGNORE;
                tok._lineBegin = p + 1;
                ++tok._line;
                ++p;
                continue;
            } else if ('*' == p[1]) {
                for (p += 2; !('*' == p[0] && '/' == p[1]); p++) {
                    if ('\n' == p[0]) {
                        tok._lineBegin = p + 1;
                        ++tok._line;
                    }
                }
                tok._tag = Token::IGNORE; ++p; ++p;
                continue;
            } else if ('=' == p[1]) {
                tok._tag = Token::DIV_ASSIGN; ++p;
            } else {
                tok._tag = Token::DIV;
            }
            ++p; break;

        case '%':
            if ('=' == p[1]) {
                tok._tag = Token::MOD_ASSIGN; ++p;
            } else if ('>' == p[1]) {
                tok._tag = '}'; ++p;
            } else if (':' == p[1]) {
                if ('%' == p[2] && ':' == p[3]) {
                    tok._tag = Token::DSHARP; p += 3;
                } else {
                    tok._tag = '#'; ++p;
                }
            } else {
                tok._tag = Token::MOD;
            }
            ++p; break;

        case '^':
            if ('=' == p[1]) {
                tok._tag = Token::XOR_ASSIGN; ++p;
            } else {
                tok._tag = Token::XOR;
            }
            ++p; break;

        case '.':
            if ('.' == p[1]) {
                if ('.' == p[2]) {
                    //ellipsis
                    tok._tag = Token::ELLIPSIS; ++p; ++p;
                } else {
                    tok._end = p;
                    tok._column = tok._begin - tok._lineBegin + 1;
                    Error(&tok, "illegal identifier '..'");
                }
            } else if (isdigit(p[1])) {	// for float constant like: '.123'
                goto constant_handler;
            } else {
                tok._tag =  Token::DOT;
            }
            ++p; break;

        case 'u':
        case 'U':
        case 'L':
            //character constant
            if ('\'' == p[1]) {
                tok._begin = p; ++p;
                goto char_handler;
            } else if ('"' == p[1]) {
                tok._begin = p; ++p;
                goto string_handler;
            } else if ('u' == p[0] && '8' == p[1] && '"' == p[2]) {
                tok._begin = p; ++p, ++p;
                goto string_handler;
            } else {
                goto letter_handler;
            }
            ++p; break;

        case '#': 
            if ('#' == p[1]) {
                tok._tag = Token::DSHARP; ++p;
            } else {
                tok._tag = '#';
            }
            ++p; break;

        case ':':
            if ('>' == p[1]) {
                tok._tag = ']'; ++p;
            } else {
                tok._tag = ':';
            }
            ++p; break;

        case '(':
        case ')':
        case '[':
        case ']':
        case '?': 
        case ',':
        case '{':
        case '}':
        case '~':
        case ';':
            tok._tag = p[0]; 
            ++p; break;

        case '\'':
            tok._begin = p;
        char_handler:
            for (++p; '\'' != p[0] && 0 != p[0]; p++) {
                if ('\\' == p[0])
                    ++p;
            }
            
            tok._tag = Token::CONSTANT;
            
            tok._end = ++p; //keep the prefix and postfix('\'')
            tok._column = tok._begin - tok._lineBegin + 1; 
            tokList.push_back(tok);
            break;

        case '"':
            tok._begin = p;
        string_handler:
            for (++p; '"' != p[0] && 0 != p[0]; p++) {
                if ('\\' == p[0]) ++p;
            }
            
            tok._tag = Token::STRING_LITERAL;
            
            tok._end = ++p; //do not trim the '"' at begin and end
            tok._column = tok._begin - tok._lineBegin + 1;
            tokList.push_back(tok);
            break;
            
        default:
        letter_handler:
            if (isalpha(p[0]) || p[0] == '_') {
                tok._begin = p;
                while (isalnum(p[0]) || p[0] == '_') {
                    ++p;
                }
                
                tok._end = p;
                tok._column = tok._begin - tok._lineBegin + 1;
                
                tok._tag = Token::KeyWordTag(tok._begin, tok._end);
                if (!Token::IsKeyWord(tok._tag)) {
                    tok._tag = Token::IDENTIFIER;
                    tokList.push_back(tok);
                } else {
                    tokList.push_back(tok);
                }
            } else if (isdigit(p[0])) {
        constant_handler:
                tok._begin = p;
                auto isInteger = ReadConstant(p);
                tok._end = p;
                tok._column = tok._begin - tok._lineBegin + 1;
                
                tok._tag = isInteger ? Token::I_CONSTANT: Token::F_CONSTANT;
                tokList.push_back(tok);
            } else {
                tok._end = p;
                tok._column = tok._begin - tok._lineBegin + 1;
                Error(&tok, "invalid character '%c'", p[0]);
            }
            ++p; break;
        }

        tok._end = p;
        tok._column = tok._begin - tok._lineBegin + 1;
        tokList.push_back(tok);
    }
}

void Lexer::ReadFile(const char* fileName)
{
    //assert(nullptr != fileName);
    FILE* fp = fopen(fileName, "r");
    if (nullptr == fp) {
        fprintf(stderr, "open file '%s' failed", fileName);
        exit(0);
    }

    long long fileSize = 0LL;
    //fseek(fp, 0, SEEK_SET);
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize > _maxSize) {
        fprintf(stderr, "source file '%s' is too big", fileName);
        exit(0);
    }

    //在tokenizer过程中需要最多向前看的步数
    auto text = new char[fileSize + 1 + _maxPredict];
    fileSize = fread((void*)text, sizeof(char), fileSize, fp);
    //text[fileSize] = 0;
    memset(&text[fileSize], 0, 1 + _maxPredict);

    _text = text;
    _fileName = fileName;
}

// Make a copy of input string
void Lexer::ReadStr(const std::string& str)
{
    size_t len = str.size();
    auto text = new char[len + 1 + _maxPredict];
    memcpy(text, &str[0], len);
    memset(&text[len], 0, 1 + _maxPredict);

    _text = text;
}

const char* Lexer::ParseName(const char* path)
{
    const char* name = path;
    while (0 != *path) {
        if ('\\' == *path || '/' == *path)
            name = path + 1;
        ++path;
    }
    
    return name;
}
