Intel Memory Latency Checker v3.12

https://www.intel.com/content/www/us/en/developer/articles/tool/intelr-memory-latency-checker.html

# Test all node
./mlc --bandwidth_matrix

# Specific node
sudo numactl --membind=1 ./mlc --peak_injection_bandwidth