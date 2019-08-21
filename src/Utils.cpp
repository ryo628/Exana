#include "Utils.hpp"

std::vector<std::string> Utils::Split( std::string& input, char delimiter)
{
    std::istringstream stream(input);
    std::string field;
    std::vector<std::string> result;
    while( getline(stream, field, delimiter) ){
        result.push_back(field);
    }
    return result;
}