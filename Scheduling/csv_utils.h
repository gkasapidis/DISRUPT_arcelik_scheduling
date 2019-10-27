//
// Created by gregkwaste on 5/6/19.
//

#ifndef CRF_RESCHEDULING_CSV_UTILS_H
#define CRF_RESCHEDULING_CSV_UTILS_H

#include <vector>
#include <string>
#include <sstream>

//CSV Parser
std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str);

#endif //CRF_RESCHEDULING_CSV_UTILS_H
