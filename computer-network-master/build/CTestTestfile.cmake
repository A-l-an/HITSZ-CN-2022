# CMake generated Testfile for 
# Source directory: D:/computer network
# Build directory: D:/computer network/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(eth_in "D:/computer network/build/eth_in.exe" "D:/computer network/testing/data/eth_in")
set_tests_properties(eth_in PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;114;add_test;D:/computer network/CMakeLists.txt;0;")
add_test(eth_out "D:/computer network/build/eth_out.exe" "D:/computer network/testing/data/eth_out")
set_tests_properties(eth_out PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;119;add_test;D:/computer network/CMakeLists.txt;0;")
add_test(arp_test "D:/computer network/build/arp_test.exe" "D:/computer network/testing/data/arp_test")
set_tests_properties(arp_test PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;124;add_test;D:/computer network/CMakeLists.txt;0;")
add_test(ip_test "D:/computer network/build/ip_test.exe" "D:/computer network/testing/data/ip_test")
set_tests_properties(ip_test PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;129;add_test;D:/computer network/CMakeLists.txt;0;")
add_test(ip_frag_test "D:/computer network/build/ip_frag_test.exe" "D:/computer network/testing/data/ip_frag_test")
set_tests_properties(ip_frag_test PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;134;add_test;D:/computer network/CMakeLists.txt;0;")
add_test(icmp_test "D:/computer network/build/icmp_test.exe" "D:/computer network/testing/data/icmp_test")
set_tests_properties(icmp_test PROPERTIES  _BACKTRACE_TRIPLES "D:/computer network/CMakeLists.txt;139;add_test;D:/computer network/CMakeLists.txt;0;")
