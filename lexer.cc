#include "lexer.h"

#include "error.h"

#include <cctype>
#include <fstream>
#include <streambuf>
#include <set>
#include <string>
#include <vector>


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

static void inline AddToken(TokenSeq& tokSeq, Token& tok)
{
    tok._ws = (!tokSeq.Empty() && tok.Column() > 1
            && tok._begin > tokSeq.Back()->_end);
    tokSeq.InsertBack(&tok);
}

void Lexer::Tokenize(TokenSeq& tokSeq)
{
    ProcessBackSlash();

    char* p = const_cast<char*>(_text->c_str());
    Token tok;
    tok._tag = Token::END;
    tok._fileName = _fileName;
    tok._line = _line;
    //tok._column = 1;
    tok._lineBegin = p;
    tok._begin = p;
    tok._end = p;

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
                    //tok._column = tok._begin - tok._lineBegin + 1;
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
        /*
        case '\\':
            if ('\n' == p[1]) {
                if ('\r' == p[2])
                    ++p;
                ++p; ++p;
            } else {
                goto error_handler;
            }
            continue;
        */
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
            //tok._column = tok._begin - tok._lineBegin + 1; 
            AddToken(tokSeq, tok);
            ++p; continue;

        case '"':
            tok._begin = p;
        string_handler:
            for (++p; '"' != p[0] && 0 != p[0]; p++) {
                if ('\\' == p[0]) ++p;
            }
            
            tok._tag = Token::STRING_LITERAL;
            
            tok._end = p + 1; //do not trim the '"' at begin and end
            //tok._column = tok._begin - tok._lineBegin + 1;
            AddToken(tokSeq, tok);
            ++p; continue;
            
        default:
        letter_handler:
            if (isalpha(p[0]) || p[0] == '_') {
                tok._begin = p;
                while (isalnum(p[0]) || p[0] == '_') {
                    ++p;
                }
                
                tok._end = p;
                //tok._column = tok._begin - tok._lineBegin + 1;
                
                tok._tag = Token::IDENTIFIER;
                AddToken(tokSeq, tok);
                //tok._tag = Token::KeyWordTag(tok._begin, tok._end);
                //if (!Token::IsKeyWord(tok._tag)) {
                //    tok._tag = Token::IDENTIFIER;
                //    AddToken(tokSeq, tok);
                //} else {
                //    AddToken(tokSeq, tok);
                //}
                continue;
            } else if (isdigit(p[0])) {
        constant_handler:
                tok._begin = p;
                auto isInteger = ReadConstant(p);
                tok._end = p;
                //tok._column = tok._begin - tok._lineBegin + 1;
                
                tok._tag = isInteger ? Token::I_CONSTANT: Token::F_CONSTANT;
                AddToken(tokSeq, tok);
                continue;
            } else {
            /*error_handler:*/
                tok._end = p;
                //tok._column = tok._begin - tok._lineBegin + 1;
                Error(&tok, "invalid character '%c'", p[0]);
            }
            ++p; break;
        }

        tok._end = p;
        //tok._column = tok._begin - tok._lineBegin + 1;
        AddToken(tokSeq, tok);
    }
}

std::string* ReadFile(const std::string& fileName)
{
    auto text = new std::string;
    
    std::ifstream file(fileName);
    if (!file.good()) {
        Error("open file '%s' failed", fileName.c_str());
    }

    file.seekg(0, std::ios::end);
    text->reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    text->assign((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
    return text;
}

void Lexer::ProcessBackSlash(void)
{
    std::string& text = *_text;
    for (size_t i = 0; i + 1 < text.size(); i++) {
        if (text[i] == '\\' && text[i + 1] == '\n') {
            std::vector<int> poss;
            auto j = i;
            for (; j + 1 < text.size() && text[j] != '\n'; j++) {
                if (text[j] == '\\' && text[j + 1] == '\n') {
                    poss.push_back(j);
                    ++j;
                }
            }
            poss.push_back(j + 1);
            size_t offset = 0;
            for (size_t k = 0; k + 1 < poss.size(); k++) {
                memcpy(&text[poss[k] - offset], &text[poss[k] + 2],
                        poss[k + 1] - poss[k]);
                offset += 2;
            }
            auto p = &text[j + 1];
            for (size_t k = 0; k + 1 < poss.size(); k++) {
                p -= 2;
                p[0] = '\n';
                p[1] = ' ';
            }
        }
    }

    text.append(3, '\0'); // guards
}
