#!/bin/bash

# This file is auto-generted by antlr4conanexample/conanfile.py by conan install ..

export ANTLR_HOME="./bin"
export ANTLR_JAR="$ANTLR_HOME/antlr-4.12.0-complete.jar"
export CLASSPATH=".:$ANTLR_JAR:$CLASSPATH"
alias antlr4="java -jar $ANTLR_JAR"
alias grun="java org.antlr.v4.gui.TestRig"

mkdir bin && cd bin && [ ! -f "antlr-4.12.0-complete.jar" ] && wget https://www.antlr.org/download/antlr-4.12.0-complete.jar
cd -

java -jar bin/antlr-4.12.0-complete.jar -o Parser -no-listener -Dlanguage=Cpp SQL.g4
