//
// Created by 胡 on 2024/1/10.
//

#ifndef DBS_OUTPUT_H
#define DBS_OUTPUT_H

#include <vector>
#include <string>

enum class OutputType {
    BATCH,
    INTERACTIVE
};

class Output {
private:
    static void batch_output(const std::vector<std::vector<std::string>> &data);
    static void interactive_output(const std::vector<std::vector<std::string>> &data);
public:
    OutputType output_type;
    explicit Output(OutputType output_type) : output_type(output_type) {}
    void output(const std::vector<std::vector<std::string>> &data) const {
        switch (output_type) {
            case OutputType::BATCH:
                batch_output(data);
                break;
            case OutputType::INTERACTIVE:
                interactive_output(data);
                break;
        }
    }
};

extern Output output_sys;
#endif//DBS_OUTPUT_H
