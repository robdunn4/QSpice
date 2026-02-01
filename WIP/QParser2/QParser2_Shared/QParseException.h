#ifndef QPARSEEXCEPTION_H
#define QPARSEEXCEPTION_H

#include <exception>
#include <string>
#include <optional>

class QParseException : public std::exception {
private:
    std::string sourceFile;
    int sourceLine;
    std::optional<int> inputFileLine;
    std::optional<std::string> inputFileLineText;
    std::string message;
    mutable std::string fullMessage;  // Cached formatted message

    void formatMessage() const {
        fullMessage = "Error in " + sourceFile + ":" + std::to_string(sourceLine);
        
        if (inputFileLine.has_value()) {
            fullMessage += " (input line " + std::to_string(*inputFileLine) + ")";
        }
        
        fullMessage += ": " + message;s
        
        if (inputFileLineText.has_value()) {
            fullMessage += "\n  Line text: " + *inputFileLineText;
        }
    }

public:
    // Constructor without input file info
    QParseException(std::string sourceFile, int sourceLine, std::string message)
        : sourceFile(std::move(sourceFile))
        , sourceLine(sourceLine)
        , inputFileLine(std::nullopt)
        , inputFileLineText(std::nullopt)
        , message(std::move(message)) {
        formatMessage();
    }

    // Constructor with input file line number only
    QParseException(std::string sourceFile, int sourceLine, int inputFileLine, std::string message)
        : sourceFile(std::move(sourceFile))
        , sourceLine(sourceLine)
        , inputFileLine(inputFileLine)
        , inputFileLineText(std::nullopt)
        , message(std::move(message)) {
        formatMessage();
    }

    // Constructor with input file line number and line text
    QParseException(std::string sourceFile, int sourceLine, int inputFileLine, 
                    std::string inputFileLineText, std::string message)
        : sourceFile(std::move(sourceFile))
        , sourceLine(sourceLine)
        , inputFileLine(inputFileLine)
        , inputFileLineText(std::move(inputFileLineText))
        , message(std::move(message)) {
        formatMessage();
    }

    const char* what() const noexcept override {
        return fullMessage.c_str();
    }

    // Accessors
    const std::string& getSourceFile() const { return sourceFile; }
    int getSourceLine() const { return sourceLine; }
    std::optional<int> getInputFileLine() const { return inputFileLine; }
    std::optional<std::string> getInputFileLineText() const { return inputFileLineText; }
    const std::string& getMessage() const { return message; }
};

// Convenience macros for throwing with automatic file/line
#define THROW_QPARSE(msg) \
    throw QParseException(__FILE__, __LINE__, msg)

#define THROW_QPARSE_WITH_INPUT(inputLine, msg) \
    throw QParseException(__FILE__, __LINE__, inputLine, msg)

#define THROW_QPARSE_WITH_INPUT_TEXT(inputLine, lineText, msg) \
    throw QParseException(__FILE__, __LINE__, inputLine, lineText, msg)

#endif // QPARSEEXCEPTION_H
	
#if 0 // examples
#include "QParseException.h"
#include <iostream>

int main() {
    try {
        // Example 1: Without input file line
        THROW_QPARSE("Invalid parameter value");
    } catch (const QParseException& e) {
        std::cout << e.what() << "\n\n";
    }

    try {
        // Example 2: With input file line number only
        int lineNum = 42;
        THROW_QPARSE_WITH_INPUT(lineNum, "Syntax error in configuration");
    } catch (const QParseException& e) {
        std::cout << e.what() << "\n\n";
    }

    try {
        // Example 3: With input file line number and line text
        int lineNum = 89;
        std::string lineText = "invalid_token = @#$%";
        THROW_QPARSE_WITH_INPUT_TEXT(lineNum, lineText, "Unexpected token");
    } catch (const QParseException& e) {
        std::cout << e.what() << std::endl;
        std::cout << "Source: " << e.getSourceFile() << std::endl;
        if (auto line = e.getInputFileLine()) {
            std::cout << "Input line: " << *line << std::endl;
        }
        if (auto text = e.getInputFileLineText()) {
            std::cout << "Line text: " << *text << std::endl;
        }
        std::cout << "\n";
    }

    try {
        // Example 4: Direct construction with all details
        throw QParseException("config_parser.cpp", 156, 23, 
                             "name = \"unterminated string", 
                             "Unterminated string literal");
    } catch (const QParseException& e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
#endif

#endif // QPARSEEXCEPTION_H