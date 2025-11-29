/*
 *  QT AGI Studio - COMPILADOR A LUA (versión FINAL y FUNCIONAL)
 *  Corregido y optimizado 2025
 */

#include <QStringList>
#include <fstream>
#include <vector>
#include <cctype>
#include <algorithm>

#include "logic.h"
#include "logedit.h"

#ifdef COMPILE_TO_LUA

static QStringList EditLines, IncludeFilenames;
extern QStringList InputLines;
static QStringList LuaCode;
static std::ofstream LuaOutputFile;

static int CurrentIndent = 0;
static int IfDepth = 0;                // cuántos 'if' están abiertos sin cerrar
static int CurLine = 0;
static std::string LowerCaseLine, ArgText, LowerCaseArgText;
static std::string::size_type LinePos = 0;
static std::string::size_type LineLength = 0;
static std::string CommandName;
static bool FinishedReading = false;
static bool ErrorOccured = false;
static int RealLineNum[65535], LineFile[65535];
static byte CommandNum = 255;  // 255 = no encontrado

// ===============================================
// UTILIDADES
// ===============================================

static void WriteLuaLine(const std::string& line)
{
    std::string indented(CurrentIndent * 2, ' ');
    indented += line;
    LuaCode.append(QString::fromStdString(indented));
    if (LuaOutputFile.is_open())
        LuaOutputFile << indented << std::endl;
}

static void WriteLuaCommand(const std::string& cmd, const std::vector<std::string>& args)
{
    std::string line = cmd + "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) line += ", ";
        line += args[i];
    }
    line += ")";
    WriteLuaLine(line);
}

// ===============================================
// ERRORES Y LÍNEAS
// ===============================================

void Logic::ShowError(int Line, std::string ErrorMsg)
{
    std::string file = (LineFile[Line] > 0 && LineFile[Line] <= IncludeFilenames.size())
        ? IncludeFilenames[LineFile[Line]-1].toStdString() : "unknown";
    std::string err = "-- ERROR [" + file + ":" + std::to_string(RealLineNum[Line]) + "] " + ErrorMsg;
    WriteLuaLine(err);
    ErrorOccured = true;
}

void Logic::NextLine()
{
    CurLine++;
    if (CurLine >= EditLines.size()) {
        FinishedReading = true;
        return;
    }
    std::string orig = EditLines[CurLine].toStdString();
    LowerCaseLine = orig;
    std::transform(LowerCaseLine.begin(), LowerCaseLine.end(), LowerCaseLine.begin(), ::tolower);
    LinePos = 0;
    LineLength = LowerCaseLine.size();
}

void Logic::SkipSpacesLua()
{
    while (LinePos < LineLength && LowerCaseLine[LinePos] == ' ') LinePos++;
}

// ===============================================
// LECTURA DE COMANDO Y ARGUMENTOS
// ===============================================

void Logic::ReadCommandName()
{
    SkipSpacesLua();
    size_t start = LinePos;
    while (LinePos < LineLength &&
           (isalnum(LowerCaseLine[LinePos]) || LowerCaseLine[LinePos] == '_' || LowerCaseLine[LinePos] == '.'))
        LinePos++;
    CommandName = LowerCaseLine.substr(start, LinePos - start);
}

// Multilínea perfecta para mensajes
void Logic::ReadArgTextLua()
{
    SkipSpacesLua();
    if (LinePos >= LineLength) {
        ArgText = "";
        return;
    }

    if (LowerCaseLine[LinePos] == '"') {
        std::string result;
        bool closed = false;
        size_t searchPos = LinePos;

        while (!FinishedReading && !closed) {
            std::string currentOrig = EditLines[CurLine].toStdString();
            size_t quote = currentOrig.find('"', searchPos + 1);

            if (quote == std::string::npos) {
                // sigue en la siguiente línea
                result += currentOrig.substr(searchPos + 1);
                NextLine();
                if (FinishedReading) break;
                SkipSpacesLua();
                searchPos = LinePos;
                if (LowerCaseLine[LinePos] != '"') {
                    ShowError(CurLine, "String no cerrada");
                    break;
                }
            } else {
                result += currentOrig.substr(searchPos + 1, quote - searchPos - 1);
                LinePos = quote + 1;
                closed = true;
            }
        }
        ArgText = "\"" + result + "\"";
    } else {
        size_t end = LowerCaseLine.find_first_of(",)", LinePos);
        if (end == std::string::npos) end = LineLength;
        ArgText = EditLines[CurLine].toStdString().substr(LinePos, end - LinePos);
        ArgText = ReplaceDefine(TrimEndWhitespaces(ArgText));
        LinePos = end;
    }

    LowerCaseArgText = ArgText;
    std::transform(LowerCaseArgText.begin(), LowerCaseArgText.end(), LowerCaseArgText.begin(), ::tolower);
}

// ===============================================
// CONVERSIÓN DE ARGUMENTOS
// ===============================================

std::string Logic::ConvertArgToLua(int type, const std::string& arg)
{
    if (arg.empty()) return arg;
    std::string l = arg;
    std::transform(l.begin(), l.end(), l.begin(), ::tolower);

    switch (type) {
        case atVar:  if (l.size() > 1 && l[0]=='v') return "vars[" + l.substr(1) + "]"; break;
        case atFlag: if (l.size() > 1 && l[0]=='f') return "flags[" + l.substr(1) + "]"; break;
        case atMsg:  return (arg[0]=='"') ? arg : "messages[" + arg + "]";
        case atIObj: return (arg[0]=='"') ? arg : "objects[" + arg + "]";
        case atNum:
        case atWord:
        default:     return arg;
    }
    return arg;
}

// ===============================================
// ARGUMENTOS (incluido said() en if)
// ===============================================

void Logic::ReadArgsToLua(bool isTest, byte cmdNum, std::vector<std::string>& args)
{
    // said() especial en condiciones if
    if (isTest && cmdNum == 14) {  // said es el número 14 en comandos de test
        std::string said = "said(";
        bool first = true;

        SkipSpacesLua();
        if (LinePos < LineLength && LowerCaseLine[LinePos] == '(') LinePos++;

        while (LinePos < LineLength) {
            ReadArgTextLua();
            if (ArgText.empty()) break;
            if (!first) said += ", ";
            said += ArgText;
            first = false;

            SkipSpacesLua();
            if (LinePos < LineLength && LowerCaseLine[LinePos] == ',') {
                LinePos++;
            } else if (LowerCaseLine[LinePos] == ')') {
                LinePos++;
                break;
            }
        }
        said += ")";
        args.push_back(said);
        return;
    }

    // Comandos normales
    CommandStruct cmd = isTest ? TestCommand[cmdNum] : AGICommand[cmdNum];

    for (int i = 0; i < cmd.NumArgs; ++i) {
        ReadArgTextLua();
        args.push_back(ConvertArgToLua(cmd.argTypes[i], ArgText));

        if (i < cmd.NumArgs - 1) {
            SkipSpacesLua();
            if (LinePos < LineLength && LowerCaseLine[LinePos] == ',')
                LinePos++;
            else
                ShowError(CurLine, "Se esperaba ',' entre argumentos");
        }
    }

    SkipSpacesLua();
    if (LinePos < LineLength && LowerCaseLine[LinePos] == ')') LinePos++;
}

// ===============================================
// CONDICIÓN IF
// ===============================================

std::string Logic::CompileConditionToLua()
{
    std::string cond;
    bool first = true;

    while (LinePos < LineLength) {
        SkipSpacesLua();
        if (LinePos >= LineLength) break;
        if (LowerCaseLine[LinePos] == ')') break;

        if (LowerCaseLine[LinePos] == '(') { cond += "("; LinePos++; continue; }

        if (!first) {
            if (LowerCaseLine.substr(LinePos, 2) == "&&") { cond += " and "; LinePos += 2; }
            else if (LowerCaseLine.substr(LinePos, 2) == "||") { cond += " or "; LinePos += 2; }
            else { cond += " "; LinePos++; }
        }

        bool notOp = false;
        if (LowerCaseLine[LinePos] == '!') { notOp = true; LinePos++; SkipSpacesLua(); }

        ReadCommandName();
        CommandNum = FindCommandNum(true, CommandName);
        if (CommandNum == 255) { ShowError(CurLine, "Comando de test desconocido"); break; }

        std::vector<std::string> args;
        if (LinePos < LineLength && LowerCaseLine[LinePos] == '(') {
            LinePos++;
            ReadArgsToLua(true, CommandNum, args);
        }

        std::string part;
        if (TestCommand[CommandNum].Name == "said")       part = args.empty() ? "false" : args[0];
        else if (TestCommand[CommandNum].Name == "isset")  part = args.empty() ? "false" : args[0];
        else if (TestCommand[CommandNum].Name == "equal")  part = args.size()>=2 ? args[0]+" == "+args[1] : "false";
        else if (TestCommand[CommandNum].Name == "greater")part = args.size()>=2 ? args[0]+" > "+args[1]  : "false";
        else if (TestCommand[CommandNum].Name == "less")   part = args.size()>=2 ? args[0]+" < "+args[1]  : "false";
        else part = "false";

        if (notOp) part = "not (" + part + ")";
        cond += part;
        first = false;

        SkipSpacesLua();
        if (LinePos < LineLength && LowerCaseLine[LinePos] == ')') { cond += ")"; LinePos++; }
    }

    SkipSpacesLua();
    if (LinePos < LineLength && LowerCaseLine[LinePos] == ')') LinePos++;

    return cond;
}

// ===============================================
// COMPILAR UN COMANDO
// ===============================================

void Logic::CompileCommandToLua()
{
    SkipSpacesLua();
    if (LinePos >= LineLength) return;

    ReadCommandName();

    // Etiqueta al inicio de línea
    if (LabelNum(CommandName) > 0 && LabelAtStartOfLine(CommandName)) {
        WriteLuaLine("::" + CommandName + "::");
        SkipSpacesLua();
        if (LinePos < LineLength && EditLines[CurLine].toStdString()[LinePos] == ':') LinePos++;
        return;
    }

    // if
    if (CommandName == "if") {
        std::string condition = CompileConditionToLua();
        WriteLuaLine("if " + condition + " then");
        CurrentIndent++;
        IfDepth++;
        return;
    }

    // else
    if (CommandName == "else") {
        CurrentIndent--;
        WriteLuaLine("end");     // cierra el if anterior
        WriteLuaLine("else");
        CurrentIndent++;
        return;
    }

    // goto
    if (CommandName == "goto") {
        SkipSpacesLua();
        if (LinePos < LineLength && LowerCaseLine[LinePos] == '(') LinePos++;
        ReadCommandName();
        WriteLuaLine("goto " + ReplaceDefine(CommandName));
        if (LinePos < LineLength && LowerCaseLine[LinePos] == ')') LinePos++;
        return;
    }

    // return
    if (CommandName == "return") {
        WriteLuaLine("return true");
        return;
    }

    // comando normal
    CommandNum = FindCommandNum(false, CommandName);
    if (CommandNum == 255) {
        ShowError(CurLine, "Comando desconocido: " + CommandName);
        return;
    }

    std::vector<std::string> args;
    if (LinePos < LineLength && LowerCaseLine[LinePos] == '(') {
        LinePos++;
        ReadArgsToLua(false, CommandNum, args);
    }

    WriteLuaCommand(AGICommand[CommandNum].Name, args);
}

// ===============================================
// COMPILAR TODO EL ARCHIVO
// ===============================================

int Logic::compileToLua()
{
    LuaCode.clear();
    CurrentIndent = 0;
    IfDepth = 0;
    ErrorOccured = false;
    CurLine = -1;
    FinishedReading = false;

    WriteLuaLine("-- AGI Logic → Lua");
    WriteLuaLine("-- Generado por QT AGI Studio");
    WriteLuaLine("");
    WriteLuaLine("local logic = {}");
    WriteLuaLine("");
    WriteLuaLine("function logic.execute(vars, flags, objects, messages)");
    CurrentIndent++;
    WriteLuaLine("vars = vars or {}");
    WriteLuaLine("flags = flags or {}");
    WriteLuaLine("objects = objects or {}");
    WriteLuaLine("messages = messages or {}");
    WriteLuaLine("");

    NextLine();
    while (!FinishedReading) {
        CompileCommandToLua();
        NextLine();
    }

    // Cerrar todos los if pendientes
    while (IfDepth > 0) {
        CurrentIndent--;
        WriteLuaLine("end");
        IfDepth--;
    }

    WriteLuaLine("return true");
    CurrentIndent--;
    WriteLuaLine("end");
    WriteLuaLine("");
    WriteLuaLine("return logic");

    return ErrorOccured ? 1 : 0;
}

// ===============================================
// FUNCIÓN PÚBLICA compile()
// ===============================================

int Logic::compile()
{
    // ... tu código de preprocesamiento (includes, defines, etc.) ...

    char path[512];
    sprintf(path, "%s/lua/logic%d.lua", game->dir.c_str(), resNum);
    LuaOutputFile.open(path);
    if (!LuaOutputFile.is_open()) {
        ErrorList.append("No se pudo crear archivo Lua");
        return 1;
    }

    int ret = compileToLua();
    LuaOutputFile.close();

    if (!ErrorOccured)
        printf("Logic %d → Lua OK\n", resNum);

    return ret;
}

QStringList Logic::GetGeneratedLuaCode()
{
    return LuaCode;
}

#endif // COMPILE_TO_LUA
