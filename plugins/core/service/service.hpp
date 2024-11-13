#pragma once

#include <source/source.hpp>
#include <vector>

/**
 * @brief Service this node
 *
 * @param[in] inputSource Source where the server will listen to
 * @param[in] outputSources Sources where to transfer the input
 * @return idk will do something about it
 */
int service(Source* inputSource, std::vector<Source *> outputSources);


int listen_source(Source* inputsource);
