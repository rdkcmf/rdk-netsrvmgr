#ifndef CONNECTIVITY_H_
#define CONNECTIVITY_H_

#include <vector>
#include <string>

/*
 * returns connectivity test endpoints as a vector of strings
 * endpoints are read from the file /opt/persistent/connectivity_test_endpoints
 * - this file must list endpoints separated by whitespace or newline
 * - if this file is empty or has only spaces/tabs, defaults will be populated
 */
std::vector<std::string> get_connectivity_test_endpoints();

bool set_connectivity_test_endpoints(const std::vector<std::string>& endpoints);

int test_connectivity(long timeout_ms);

int test_connectivity(long timeout_ms, const std::vector<std::string> &endpoints);

#endif /* CONNECTIVITY_H_ */
