from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout

script = """#!/bin/bash

# This file is auto-generted by antlr4conanexample/conanfile.py by conan install ..

export ANTLR_HOME="./bin"
export ANTLR_JAR="$ANTLR_HOME/antlr-VERSION-complete.jar"
export CLASSPATH=".:$ANTLR_JAR:$CLASSPATH"
alias antlr4="java -jar $ANTLR_JAR"
alias grun="java org.antlr.v4.gui.TestRig"

mkdir bin && cd bin && [ ! -f "antlr-VERSION-complete.jar" ] && wget https://www.antlr.org/download/antlr-VERSION-complete.jar
cd -

java -jar bin/antlr-VERSION-complete.jar -o Parser -no-listener -Dlanguage=Cpp SQL.g4
"""

class AntLR4Example(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps" , "CMakeToolchain"
    antlr4_version = "4.12.0"
    testing = []
    requires = "antlr4-cppruntime/"+antlr4_version

    def layout(self):
        cmake_layout(self)

    def package(self):
        self.copy("*.h", "include", "build/include")

    def requirements(self):
        antlr4_version_file = open("src/regenerate_parser.sh","w")
        antlr4_version_file.write(script.replace('VERSION',self.antlr4_version))
        antlr4_version_file.close()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
