//
// Created by 胡 on 2024/1/10.
//

#ifndef DBS_ERROR_H
#define DBS_ERROR_H

#include <exception>
#include <string>

class Error : public std::exception {
private:
    std::string msg;
public:
    explicit Error(std::string msg) : msg(std::move(msg)) {}
    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }
};

class UnimplementedError : public Error {
public:
    explicit UnimplementedError(const std::string& msg= "") : Error(msg+"Unimplemented!") {}
};


#endif//DBS_ERROR_H
